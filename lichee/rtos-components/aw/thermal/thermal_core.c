#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <hal_cmd.h>
#include <FreeRTOS.h>
#include <hal_timer.h>

#include "thermal.h"
#include "thermal_sensor.h"
#include "thermal_external.h"

#define thermal_zone_containerof(ptr_module) \
        __containerof(ptr_module, struct thermal_zone, node)

static struct list_head thermal_zone_list = LIST_HEAD_INIT(thermal_zone_list);

int thermal_log_level = 0x3;

/* Temperature Detection */
static void thermal_zome_get_temperature(struct thermal_zone_device *tzd, int *temp)
{
	int ret;

	ret = tzd->ops->get_temp(tzd->priv, temp);

	hal_spin_lock(&tzd->zone->lock);
	/* only if temp < critical temp can temp be overwritten by emulation temp */
	if (tzd->zone->func_switch & THERMAL_EMUL_TEMP_SWITCH_MASK) {
		if (tzd->zone->func_switch & THERMAL_CRIT_TEMP_SWITCH_MASK) {
			if (*temp < tzd->zone->crit_temperature)
				*temp = tzd->zone->emul_temperature;
		} else
			*temp = tzd->zone->emul_temperature;
	}

	hal_spin_unlock(&tzd->zone->lock);
}

static void update_temperature(struct thermal_zone_device *tzd)
{
	int ret;
	int temp;

	if (!tzd->ops || !tzd->ops->get_temp || !tzd->zone)
		return;
	thermal_zome_get_temperature(tzd, &temp);

	hal_spin_lock(&tzd->zone->lock);
	tzd->zone->last_temperature = tzd->zone->temperature;
	tzd->zone->temperature = temp;
	hal_spin_unlock(&tzd->zone->lock);

	if (tzd->zone->last_temperature == THERMAL_TEMP_INVALID)
		thermal_dbg("last temperature N/A, current temperature %d\n", tzd->zone->temperature);
	else
		thermal_dbg("last temperature %d, current temperature %d\n",
			tzd->zone->last_temperature, tzd->zone->temperature);
}

static void handler_thermal_temperature(struct thermal_zone_device *tzd)
{
	if (!(tzd->zone->func_switch & THERMAL_CRIT_TEMP_SWITCH_MASK)
		|| (tzd->zone->temperature < tzd->zone->crit_temperature))
		return;

	thermal_warn("critical temperature reached(%d C),shutting down\n", tzd->zone->temperature / 1000);
	/* FIXME: delay for warning log to print, it should not be done*/
	hal_mdelay(3);
	thermal_warn("\n");
	tzd->ops->shutdown();
}

static void thermal_zone_device_check_task(void *data)
{
	struct thermal_zone_device *tzd = data;
	hal_status_t stat;
	uint32_t mode;

	while (1) {
		if (xQueueReceive(tzd->xQueue, &mode, portMAX_DELAY) != pdPASS)
			continue;
		hal_spin_lock(&tzd->zone->lock);
		stat = osal_timer_stop(tzd->zone->polling_timer);
		if (stat) {
			thermal_err("thermal zone %d stop polling timer failed\n", tzd->zone->id);
			hal_spin_unlock(&tzd->zone->lock);
			continue;
		}
		hal_spin_unlock(&tzd->zone->lock);

		update_temperature(tzd);

		handler_thermal_temperature(tzd);

		hal_spin_lock(&tzd->zone->lock);
		stat = osal_timer_start(tzd->zone->polling_timer);
		if (stat) {
			thermal_err("thermal zone %d start polling timer failed\n", tzd->zone->id);
			hal_spin_unlock(&tzd->zone->lock);
			continue;
		}
		hal_spin_unlock(&tzd->zone->lock);
	}
}

/* could not sleep inside, just send queue */
static void thermal_zone_device_check(void *data)
{
	struct thermal_zone_device *tzd;
	uint32_t mode = 0;
	BaseType_t xReturned;

	if (!data)
		return;

	tzd = data;
	xReturned = xQueueSend(tzd->xQueue, &mode, 0);
	if (xReturned != pdPASS)
		thermal_err("thermal zone %d: periodic check send queue error\n", tzd->zone->id);
}

/* thermal zone ops */
static int thermal_zone_set_emul_temp(void *data, const int emul_temp)
{
	struct thermal_zone *tz = data;

	hal_spin_lock(&tz->lock);
	tz->emul_temperature = emul_temp;
	hal_spin_unlock(&tz->lock);

	return 0;
}

static int thermal_zone_set_crit_temp(void *data, const int crit_temp)
{
	struct thermal_zone *tz = data;

	hal_spin_lock(&tz->lock);
	tz->crit_temperature = crit_temp;
	hal_spin_unlock(&tz->lock);

	return 0;
}

static int thermal_zone_set_func_switch(void *data, const int func_switch_mask)
{
	struct thermal_zone *tz = data;

	hal_spin_lock(&tz->lock);
	tz->func_switch = func_switch_mask;
	hal_spin_unlock(&tz->lock);

	return 0;
}

static struct thermal_zone_ops tz_ops = {
	.set_emul_temp = thermal_zone_set_emul_temp,
	.set_crit_temp = thermal_zone_set_crit_temp,
	.set_func_switch = thermal_zone_set_func_switch,
};

/* thermal init */
struct thermal_zone_device *thermal_zone_device_register(char *zone_name,
							struct thermal_zone_device_ops *ops,
							void *priv)
{
	struct thermal_zone *tz;
	struct thermal_zone_device *tzd;
	struct list_head *node = NULL;
	struct list_head *storage = NULL;
	BaseType_t xReturned;
	hal_status_t stat;
	int id = 0;
	char tmp_name[] = "thermal_timer";
	char timer_name[20];

	if (!zone_name || !ops || !priv)
		return NULL;

	tzd = malloc(sizeof(struct thermal_zone_device));
	if (!tzd) {
		thermal_err("thermal zone device mallock failed\n");
		return NULL;
	}

	tz = malloc(sizeof(struct thermal_zone));
	if (!tz) {
		thermal_err("thermal zone mallock failed\n");
		goto free_tzd;
	}

	list_for_each_safe(node, storage, &thermal_zone_list) {
		id++;
	}
	snprintf(timer_name, sizeof(timer_name), "%s%d", tmp_name, id);

	hal_spin_lock_init(&tz->lock);
	hal_spin_lock(&tz->lock);

	tz->temperature = THERMAL_TEMP_INVALID;
	tz->emul_temperature = 0;
	tz->crit_temperature = THERMAL_TEMP_CRITICAL;
	tz->func_switch = THERMAL_CRIT_TEMP_SWITCH_MASK;

	tz->type = zone_name;
	tz->id = id;
	tz->zone_dev = tzd;
	tz->ops = &tz_ops;
	list_add(&tz->node, &thermal_zone_list);
	tz->polling_interval = POLLING_INTERVAL;
	/* timer work periodically, and work in thread context if flag is OSAL_TIMER_FLAG_SOFT_TIMER.*/
	tz->polling_timer = osal_timer_create(timer_name, thermal_zone_device_check,
						tz->zone_dev, tz->polling_interval,
						OSAL_TIMER_FLAG_PERIODIC);
	if (!tz->polling_timer) {
		thermal_err("thermal zone %d create timer failed\n", id);
		hal_spin_unlock(&tz->lock);
		goto free_tz_tzd;
	}

	tzd->priv = priv;
	tzd->ops = ops;
	tzd->zone = tz;

	tzd->xQueue = xQueueCreate( 1, sizeof( uint32_t ) );
	if (NULL == tzd->xQueue) {
		thermal_err("thermal zone %d device create queue failed\n", id);
		hal_spin_unlock(&tz->lock);
		goto free_timer_tz_tzd;
	}

	xReturned = xTaskCreate(thermal_zone_device_check_task, zone_name, (10 * 1024) / sizeof(StackType_t), tzd,
		OS_PRIORITY_REAL_TIME, (TaskHandle_t * const)&tzd->xHandle);
	if (pdPASS != xReturned) {
		thermal_err("thermal zone %d device create thread failed\n", id);
		hal_spin_unlock(&tz->lock);
		goto free_queue_timer_tz_tzd;
	}

	hal_spin_unlock(&tz->lock);

	thermal_dbg("add thermal_zone %d: %s\n", tz->id, tz->type);

	return tzd;

free_queue_timer_tz_tzd:
	vQueueDelete(tzd->xQueue);

free_timer_tz_tzd:
	stat = osal_timer_delete(tz->polling_timer);
	if (stat)
		thermal_err("thermal zone %d timer delete failed, return %d\n", id, stat);

free_tz_tzd:
	hal_spin_lock_deinit(&tz->lock);
	free(tz);

free_tzd:
	free(tzd);
	return NULL;

}

int thermal_init(void)
{
	int ret = 0;
	hal_status_t stat = 0;
	struct list_head *node = NULL;
	struct list_head *storage = NULL;
	struct thermal_zone *tz = NULL;

	ret = thermal_sensor_init();
	if (ret) {
		thermal_err("thermal sensor init failed, return %d\n", ret);
		return ret;
	}

	list_for_each_safe(node, storage, &thermal_zone_list) {
		tz = thermal_zone_containerof(node);
		stat = osal_timer_start(tz->polling_timer);
		if (stat) {
			thermal_err("thermal zone %d timer start failed, return %d\n", tz->id, stat);
			ret = stat;
			return ret;
		}
	}

	return ret;
}

/* thermal deinit */
int thermal_zone_device_unregister(struct thermal_zone_device *zone_dev)
{
	hal_status_t stat;

	if (!zone_dev || !zone_dev->zone)
		return -EINVAL;

	hal_spin_lock(&zone_dev->zone->lock);
	if (zone_dev->xHandle)
		vTaskDelete(zone_dev->xHandle);
	vQueueDelete(zone_dev->xQueue);
	stat = osal_timer_stop(zone_dev->zone->polling_timer);
	if (stat)
		thermal_err("thermal zone %d timer stop failed, return %d\n", zone_dev->zone->id, stat);
	stat = osal_timer_delete(zone_dev->zone->polling_timer);
	if (stat)
		thermal_err("thermal zone %d timer delete failed, return %d\n", zone_dev->zone->id, stat);
	list_del(&zone_dev->zone->node);
	hal_spin_unlock(&zone_dev->zone->lock);
	hal_spin_lock_deinit(&zone_dev->zone->lock);
	free(zone_dev->zone);
	zone_dev->zone = NULL;
	free(zone_dev);

	return 0;
}

void thermal_zone_deinit(void)
{
	thermal_sensor_deinit();
}

/* thermal cmd */
void cmd_thermal_usage(void)
{
	char buff[] = \
	"Usage Help:\n"
	"-c: set critical temperature.\n"
	"-e: set emulation temperature.\n"
	"-f: disable/enable function:\n"
	"    0x1 - Over-temperature protection; 0x2 - emulation temperature;\n"
	"    enable all function: thermal -f 3\n"
	"-g: get current temperature.\n"
	"-l: set thermal log level:\n"
	"    0x1 - ERR, 0x2 - WARN, 0x4 - DBG, 0x8 - INFO;\n"
	"    open all logs: thermal -l 15\n"
	"-h: print Usage Help.\n"
	"\n";

	printf("%s", buff);
}

static int cmd_thermal(int argc, char **argv)
{
	int ret;
	int opt;
	int value;
	struct thermal_zone *tz;
	struct list_head *node = NULL;
	struct list_head *storage = NULL;

	while ((opt = getopt(argc, argv, "c:e:f:gl:h")) != -1) {
		switch (opt) {
		case 'c':
			value = atoi(optarg);
			list_for_each_safe(node, storage, &thermal_zone_list) {
				tz = thermal_zone_containerof(node);
				ret = tz->ops->set_crit_temp(tz, value);
				if (ret)
					thermal_err("set thermal zone%d crit_temperature failed, return %d\n", tz->id, ret);
				else
					thermal_warn("set thermal zone%d crit_temperature: %d\n", tz->id, value);
			}
			break;
		case 'e':
			value = atoi(optarg);
			if (value )
			list_for_each_safe(node, storage, &thermal_zone_list) {
				tz = thermal_zone_containerof(node);
				ret = tz->ops->set_emul_temp(tz, value);
				if (ret)
					thermal_err("set thermal zone%d emul_temperature failed, return %d\n", tz->id, ret);
				else
					thermal_warn("set thermal zone%d emul_temperature: %d\n", tz->id, value);
			}
			break;
		case 'f':
			value = atoi(optarg);
			if ((value < 0) || (value > 0xf)) {
				thermal_err("invalid func_switch value\n");
				break;
			}

			list_for_each_safe(node, storage, &thermal_zone_list) {
				tz = thermal_zone_containerof(node);
				ret = tz->ops->set_func_switch(tz, value);
				if (ret)
					thermal_err("set thermal zone%d func_switch failed, return %d\n", tz->id, ret);
				else
					thermal_warn("set thermal zone%d func_switch: 0x%x\n", tz->id, value);
			}
			break;
		case 'g':
			list_for_each_safe(node, storage, &thermal_zone_list) {
				tz = thermal_zone_containerof(node);
				thermal_zome_get_temperature(tz->zone_dev, &value);
				thermal_warn("thermal zone%d: %d\n", tz->id, value);
			}
			break;
		case 'l':
			value = atoi(optarg);
			if ((value >= 0) && (value <= 0xf)) {
				thermal_log_level = value;
				thermal_warn("set thermal log level: 0x%x\n", thermal_log_level);
			} else
				thermal_err("invalid log level: %d\n", value);
			break;
		case 'h':
		default:
			cmd_thermal_usage();
			break;
		}
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_thermal, thermal, thermal tools)

/* External interface */
int thermal_external_get_zone_number(void)
{
	struct list_head *node = NULL;
	struct list_head *storage = NULL;
	struct thermal_zone *tz;
	int num = 0;

	list_for_each_safe(node, storage, &thermal_zone_list) {
		num++;
	}

	return num;
}

int thermal_external_get_temp(int *id_buf, int *temp_buf, int num)
{
	struct list_head *node = NULL;
	struct list_head *storage = NULL;
	struct thermal_zone *tz;
	int temp;
	int i = 0;

	if (!id_buf || !temp_buf || (num > thermal_external_get_zone_number()))
		return -EINVAL;


	list_for_each_safe(node, storage, &thermal_zone_list) {
		tz = thermal_zone_containerof(node);
		thermal_zome_get_temperature(tz->zone_dev, &temp);
		id_buf[i] = tz->id;
		temp_buf[i] = temp;
		i++;
		if (i == num)
			break;
	}

	return 0;
}
