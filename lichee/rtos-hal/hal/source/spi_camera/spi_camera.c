#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sunxi_hal_twi.h"
#include "hal_timer.h"
#include "hal_time.h"
#include "hal_log.h"

#include "hal/sdmmc/hal/hal_def.h"
#include "hal_atomic.h"

#include "hal_gpio.h"
#include "hal_mutex.h"
#include "hal_reset.h"

#include <hal_cmd.h>
#include <hal_mem.h>
#include <sunxi_hal_spi.h>

#include "spi_sensor/spi_sensor.h"
#include "spi_camera.h"

#if CONFIG_DRIVER_SYSCONFIG
#include <hal_cfg.h>
#include <script.h>
#endif

struct sensor_dev_cfg_t {
    int8_t name[32];
    uint32_t max_speed;
    uint8_t twi_id;
    uint8_t twi_addr;
    uint8_t spi_id;
    int pwdn_pin;
    int irq_pin;
};
static struct sensor_dev_cfg_t sensor_dev_cfg;

static uint32_t irq;
static spi_camera_private *gspi_camera_priv;
static spi_private *gspi_private;
static int spi_camera_xfer(void);

static spi_camera_private *spi_camera_getpriv()
{
    return gspi_camera_priv;
}

static void spi_jpeg_setpriv(spi_camera_private *spi_camera_priv)
{
    gspi_camera_priv = spi_camera_priv;
}

static spi_private *spi_getpriv()
{
    if (gspi_private == NULL) {
        spi_camera_err("spi not probe");
        return NULL;
    } else
        return gspi_private;
}

static void spi_setpriv(spi_private *spi_priv)
{
    gspi_private = spi_priv;
}

HAL_Status hal_spi_set_fmt(struct spi_fmt *spi_output_fmt)
{
    spi_private *spi_priv = spi_getpriv();
    spi_camera_private *spi_camera_priv = spi_camera_getpriv();
    struct sensor_win_size *sensor_win_sizes;

    sensor_win_sizes = spi_priv->sensor_func.sensor_win_sizes;

    if (spi_output_fmt->width > sensor_win_sizes->width || spi_output_fmt->height > sensor_win_sizes->height) {
        spi_camera_err("output size morn than sensor size, sensor:%d*%d, output:%d*%d\n",
                    sensor_win_sizes->width, sensor_win_sizes->height,
                    spi_output_fmt->width, spi_output_fmt->height);
    }

    spi_priv->spi_cfg.hor_len = spi_output_fmt->width;
    spi_priv->spi_cfg.ver_len = spi_output_fmt->height;

    spi_camera_priv->output_fmt.height = spi_output_fmt->height;
    spi_camera_priv->output_fmt.width = spi_output_fmt->width;

    return 0;
}

HAL_Status hal_sensor_stream(unsigned int on)
{
    spi_private *spi_priv = spi_getpriv();
    HAL_Status ret;

    if (on)
        ret = spi_priv->sensor_func.init(&spi_priv->sensor_cfg);
    else {
        ret = spi_priv->sensor_func.deinit(&spi_priv->sensor_cfg);
    }

    return ret;
}


static HAL_Status spi_dev_cfg_parse(void)
{
#ifdef CONFIG_DRIVER_SYSCONFIG
    int value;
    int ret = -1;
    user_gpio_set_t gpio_pwdn = {0};
    user_gpio_set_t gpio_irq = {0};

    //sensor max speed hz
    ret = hal_cfg_get_keyvalue(SPI_SENSOR_BOARD, SPI_SENSOR_MAX_SPEED,
        (int32_t *)&value, 1);
    if (ret) {
        spi_camera_dbg("spi_dev:{%s} miss", SPI_SENSOR_MAX_SPEED);
        return ret;
    }
    sensor_dev_cfg.max_speed = value;

    //sensor twi id
    ret = hal_cfg_get_keyvalue(SPI_SENSOR_BOARD, SPI_SENSOR_TWI_ID,
        (int32_t *)&value, 1);
    if (ret) {
        spi_camera_dbg("spi_dev:{%s} miss", SPI_SENSOR_TWI_ID);
        return ret;
    }
    sensor_dev_cfg.twi_id = value;

    //sensor twi addr
    ret = hal_cfg_get_keyvalue(SPI_SENSOR_BOARD, SPI_SENSOR_TWI_ADDR,
        (int32_t *)&value, 1);
    if (ret) {
        spi_camera_dbg("spi_dev:{%s} miss", SPI_SENSOR_TWI_ADDR);
        return ret;
    }
    sensor_dev_cfg.twi_addr = value;

    //sensor spi id
    ret = hal_cfg_get_keyvalue(SPI_SENSOR_BOARD, SPI_SENSOR_SPI_ID,
        (int32_t *)&value, 1);
    if (ret) {
        spi_camera_dbg("spi_dev:{%s} miss", SPI_SENSOR_SPI_ID);
        return ret;
    }
    sensor_dev_cfg.spi_id = value;

    //sensor pwdn
    ret = hal_cfg_get_keyvalue(SPI_SENSOR_BOARD, SPI_SENSOR_PWDN,
        (int32_t *)&gpio_pwdn, sizeof(user_gpio_set_t) >> 2);
    if (ret) {
        spi_camera_dbg("spi_dev:{%s} miss", SPI_SENSOR_PWDN);
        return ret;
    }
    sensor_dev_cfg.pwdn_pin = (gpio_pwdn.port - 1) * PINS_PER_BANK + gpio_pwdn.port_num;

    //sensor irq
    ret = hal_cfg_get_keyvalue(SPI_SENSOR_BOARD, SPI_SENSOR_IRQ,
        (int32_t *)&gpio_irq, sizeof(user_gpio_set_t) >> 2);
    if (ret) {
        spi_camera_dbg("spi_dev:{%s} miss", SPI_SENSOR_IRQ);
        return ret;
    }
    sensor_dev_cfg.irq_pin = (gpio_irq.port - 1) * PINS_PER_BANK + gpio_irq.port_num;
#else
    sensor_dev_cfg.max_speed = SPI_SENSOR_MAX_SPEED_NONE;
    sensor_dev_cfg.twi_id = SPI_SENSOR_TWI_ID_NONE;
    sensor_dev_cfg.twi_addr = SPI_SENSOR_TWI_ADDR_NONE;
    sensor_dev_cfg.spi_id = SPI_SENSOR_SPI_ID_NONE;
    sensor_dev_cfg.pwdn_pin = SPI_SENSOR_PWDN_NONE;
#endif

    spi_camera_dbg("max speed=%ld", sensor_dev_cfg.max_speed);
    spi_camera_dbg("sensor twi id=%d", sensor_dev_cfg.twi_id);
    spi_camera_dbg("sensor twi addr=%x", sensor_dev_cfg.twi_addr);
    spi_camera_dbg("sensor spi id=%d", sensor_dev_cfg.spi_id);
    spi_camera_dbg("sensor pwdn=%d", sensor_dev_cfg.pwdn_pin);
    spi_camera_dbg("sensor irq=%d", sensor_dev_cfg.irq_pin);
    return HAL_OK;
}

static int hal_spi_probe(void)
{
    spi_private *spi_priv;

    spi_priv = malloc(sizeof(spi_private));
    if (!spi_priv) {
        spi_camera_err("malloc faild\n");
        return -1;
    } else {
        memset(spi_priv, 0, sizeof(spi_private));
        spi_setpriv(spi_priv);
    }

    spi_priv->lock = hal_mutex_create();
    if (!spi_priv->lock) {
        spi_camera_err("mutex create fail\n");
        goto emalloc;
    }

    spi_priv->sensor_cfg.twi_port = sensor_dev_cfg.twi_id;
    spi_priv->sensor_cfg.twi_addr = sensor_dev_cfg.twi_addr;
    spi_priv->sensor_cfg.pwcfg.pwdn_pin = sensor_dev_cfg.pwdn_pin;

    spi_priv->sensor_func.init = SPI_SENSOR_FUNC_INIT;
    spi_priv->sensor_func.deinit = SPI_SENSOR_FUNC_DEINIT;
    spi_priv->sensor_func.sensor_win_sizes = &SPI_SENSOR_FUNC_WIN_SIZE;

    return 0;
emalloc:
    free(spi_priv);
    spi_setpriv(NULL);

    return -1;
}

static void hal_spi_remove(void)
{
    spi_private *spi_priv = spi_getpriv();
    if (!spi_priv) {
        spi_camera_err("spi_priv not init\n");
        return;
    }

    hal_mutex_delete(spi_priv->lock);
    free(spi_priv);
    spi_setpriv(NULL);
}

int hal_spi_sensor_reqbuf(unsigned int count)
{
    spi_camera_private *spi_camera_priv = spi_camera_getpriv();
    unsigned int size;
    unsigned int spi_size;
    int i;

    if (count < 2) {
        count = 3;
    }
    spi_camera_priv->spi_mem_count = count;

    spi_size = spi_camera_priv->output_fmt.width * spi_camera_priv->output_fmt.height;

    hal_mutex_lock(spi_camera_priv->lock);
    for (i = 0; i < count; i++) {
        spi_camera_priv->spi_mem[i].index = i;
        spi_camera_priv->spi_mem[i].buf.size = spi_size;

        spi_camera_priv->spi_mem[i].buf.addr = hal_malloc_coherent(spi_size);

        if (spi_camera_priv->spi_mem[i].buf.addr == NULL) {
            spi_camera_err("spi malloc size :%d failed\n", spi_size);
            return -1;
        }

        memset(spi_camera_priv->spi_mem[i].buf.addr, 0, spi_size);
        spi_camera_dbg("i = %d addr = 0x%08x\n", i, spi_camera_priv->spi_mem[i].buf.addr);
        list_add_tail(&spi_camera_priv->spi_mem[i].list, &spi_camera_priv->spi_active);
    }

    hal_mutex_unlock(spi_camera_priv->lock);

    return 0;
}

int hal_spi_sensor_freebuf(void)
{
    spi_camera_private *spi_camera_priv = spi_camera_getpriv();
    struct spi_ipeg_mem *spi_mem;
    int i;

    if (spi_camera_priv->spi_stream) {
        spi_camera_err("please stream off spi and jpef frist\n");
        return -1;
    }

    hal_mutex_lock(spi_camera_priv->lock);
    while (!list_empty(&spi_camera_priv->spi_done)) {
        spi_mem = list_entry(spi_camera_priv->spi_done.next, struct spi_ipeg_mem, list);
        list_del(&spi_mem->list);
        spi_camera_dbg("spi buf%d delect from done queue\n", spi_mem->index);
    }

    while (!list_empty(&spi_camera_priv->spi_active)) {
        spi_mem = list_entry(spi_camera_priv->spi_active.next, struct spi_ipeg_mem, list);
        list_del(&spi_mem->list);
        spi_camera_dbg("spi buf%d delect from active queue\n", spi_mem->index);
    }

    for (i = 0; i < spi_camera_priv->spi_mem_count; i++) {
        hal_free_coherent(spi_camera_priv->spi_mem[i].buf.addr);
    }

    hal_mutex_unlock(spi_camera_priv->lock);

    return 0;
}

struct spi_ipeg_mem *hal_spi_dqbuf(struct spi_ipeg_mem *spi_mem, unsigned int timeout_msec)
{
    spi_camera_private *spi_camera_priv = spi_camera_getpriv();
    int ret;
    uint32_t cpu_sr;

    spi_camera_priv->sem = 1;
    ret = hal_sem_timedwait(spi_camera_priv->xSemaphore_rx, timeout_msec);
    spi_camera_priv->sem = 0;
    if (ret != 0)
        return NULL;

    spi_camera_xfer();

    hal_mutex_lock(spi_camera_priv->lock);
    if (list_empty(&spi_camera_priv->spi_done)) {
        spi_camera_err("spi done queue is empty\n");
        hal_mutex_unlock(spi_camera_priv->lock);
        return NULL;
    }
    cpu_sr = hal_spin_lock_irqsave(&spi_camera_priv->slock);
    spi_mem = list_entry(spi_camera_priv->spi_done.next, struct spi_ipeg_mem, list);
    hal_spin_unlock_irqrestore(&spi_camera_priv->slock, cpu_sr);

    hal_mutex_unlock(spi_camera_priv->lock);

    return spi_mem;
}

void hal_spi_qbuf(void)
{
    spi_camera_private *spi_camera_priv = spi_camera_getpriv();
    struct spi_ipeg_mem *spi_mem;
    uint32_t cpu_sr;

    hal_mutex_lock(spi_camera_priv->lock);
    if (list_empty(&spi_camera_priv->spi_done)) {
        spi_camera_err("spi done queue is empty\n");
        hal_mutex_unlock(spi_camera_priv->lock);
        return;
    }

    cpu_sr = hal_spin_lock_irqsave(&spi_camera_priv->slock);

    spi_mem = list_entry(spi_camera_priv->spi_done.next, struct spi_ipeg_mem, list);
    list_move_tail(&spi_mem->list, &spi_camera_priv->spi_active);

    hal_spin_unlock_irqrestore(&spi_camera_priv->slock, cpu_sr);
    hal_mutex_unlock(spi_camera_priv->lock);
}

void disable_irq()
{
    hal_gpio_irq_disable(irq);
}

void enable_irq()
{
    hal_gpio_irq_enable(irq);
}

static int spi_camera_xfer(void)
{
    int ret = -1;
    uint32_t cpu_sr;
    struct spi_ipeg_mem *spi_mem;
    hal_spi_master_transfer_t tr;
    spi_camera_private *spi_camera_priv = spi_camera_getpriv();

    spi_mem = list_entry(spi_camera_priv->spi_active.next, struct spi_ipeg_mem, list);

    tr.rx_buf = spi_mem->buf.addr;
    tr.rx_len = spi_mem->buf.size;
    tr.tx_buf = NULL;
    tr.tx_len = 0;
    tr.dummy_byte = 0;
    tr.tx_single_len = spi_mem->buf.size;
    tr.rx_nbits = SPI_NBITS_SINGLE;
    ret = hal_spi_xfer(sensor_dev_cfg.spi_id, &tr);

    if((&spi_camera_priv->spi_active) != spi_camera_priv->spi_active.next->next) {
        spi_mem = list_entry(spi_camera_priv->spi_active.next, struct spi_ipeg_mem, list);
        list_move_tail(&spi_mem->list, &spi_camera_priv->spi_done);
    }

    return ret;
}

static hal_irqreturn_t spi_jpeg_isr(void *data)
{
    gpio_data_t irq_state;
    spi_camera_private *spi_camera_priv = spi_camera_getpriv();
    uint32_t cpu_sr;

    if (!spi_camera_priv->spi_stream)
    {
        spi_camera_err("please open camera\n");
        return -1;
    }

    cpu_sr = hal_spin_lock_irqsave(&spi_camera_priv->slock);
    if (spi_camera_priv->sem)
        hal_sem_post(spi_camera_priv->xSemaphore_rx);

    hal_spin_unlock_irqrestore(&spi_camera_priv->slock, cpu_sr);
    return 0;
}

HAL_Status hal_spi_s_stream(unsigned int on)
{
    spi_private *spi_priv = spi_getpriv();

    if (on)
        hal_gpio_irq_enable(irq);
    else
        hal_gpio_irq_disable(irq);

    return 0;
}

void hal_spi_sensor_s_stream(unsigned int on)
{
    spi_camera_private *spi_camera_priv = spi_camera_getpriv();

    if (on) {
        spi_camera_priv->spi_stream = 1;
        hal_spi_s_stream(on);
        hal_usleep(200);
        hal_sensor_stream(on);
    } else {
        hal_sensor_stream(on);
        hal_spi_s_stream(on);
        spi_camera_priv->spi_stream = 0;
    }
}

HAL_Status hal_spi_sensor_probe(void)
{
    spi_camera_private *spi_camera_priv;
    hal_spi_master_config_t cfg;
    int ret = -1;

    spi_camera_priv = malloc(sizeof(spi_camera_private));
    if (!spi_camera_priv) {
        spi_camera_err("spi sensor malloc faild\n");
        return HAL_ERROR;
    } else {
        memset(spi_camera_priv, 0, sizeof(spi_camera_private));
        spi_jpeg_setpriv(spi_camera_priv);
    }

    spi_camera_priv->lock = hal_mutex_create();
    if (!spi_camera_priv->lock) {
        spi_camera_err("mutex create fail\n");
        goto emalloc;
    }

    hal_waitqueue_head_init(&spi_camera_priv->spi_waitqueue);
    hal_spin_lock_init(&spi_camera_priv->slock);
    spi_camera_priv->xSemaphore_rx = hal_sem_create(0);
    if (NULL == spi_camera_priv->xSemaphore_rx) {
        spi_camera_err("creating semaphore_rx failed.\n");
        goto emalloc;
    }

    INIT_LIST_HEAD(&spi_camera_priv->spi_active);
    INIT_LIST_HEAD(&spi_camera_priv->spi_done);

    if (spi_dev_cfg_parse()) {
        spi_camera_err("spi camera dev config parse failed\n");
        goto emalloc;
    }

    hal_spi_probe();

    hal_gpio_set_pull(sensor_dev_cfg.irq_pin, GPIO_PULL_UP);
    hal_gpio_pinmux_set_function(sensor_dev_cfg.irq_pin, GPIO_MUXSEL_EINT);
    hal_gpio_set_direction(sensor_dev_cfg.irq_pin, GPIO_DIRECTION_INPUT);

    cfg.clock_frequency = sensor_dev_cfg.max_speed;
    cfg.slave_port = HAL_SPI_MASTER_SLAVE_0;
    cfg.cpha = HAL_SPI_MASTER_CLOCK_PHASE1;
    cfg.cpol = HAL_SPI_MASTER_CLOCK_POLARITY0;
    cfg.spol = HAL_SPI_MASTER_SPOL_HIGH;
    cfg.sip = 0;
    cfg.slave = true;
    hal_spi_init(sensor_dev_cfg.spi_id, &cfg);

    /*request spi irq*/
    ret = hal_gpio_to_irq(sensor_dev_cfg.irq_pin, &irq);
    if(ret < 0)
        spi_camera_err("gpio to irq error, irq number %d error num: %d\n", irq, ret);

    ret = hal_gpio_irq_request(irq, spi_jpeg_isr, IRQ_TYPE_EDGE_FALLING, NULL);
    if (ret < 0)
    {
        spi_camera_err("request irq error, irq num:%d error num: %d", irq, ret);
        return -1;
    } else {
        spi_camera_info("hal_gpio_irq_request API success!\n");
    }

    hal_gpio_irq_disable(irq);

    return HAL_OK;
freemutex:
    hal_mutex_delete(spi_camera_priv->lock);

emalloc:
    free(spi_camera_priv);
    spi_jpeg_setpriv(NULL);

    return HAL_ERROR;
}

HAL_Status hal_spi_sensor_remove(void)
{
    spi_camera_private *spi_camera_priv = spi_camera_getpriv();

    hal_gpio_irq_free(irq);
    hal_spi_deinit(sensor_dev_cfg.spi_id);
    hal_spi_remove();

    hal_spin_lock_deinit(&spi_camera_priv->slock);
    hal_waitqueue_head_deinit(&spi_camera_priv->spi_waitqueue);
    hal_sem_delete(spi_camera_priv->xSemaphore_rx);
    hal_mutex_delete(spi_camera_priv->lock);

    free(spi_camera_priv);
    spi_jpeg_setpriv(NULL);

    return HAL_OK;
}
