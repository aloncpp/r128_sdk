#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <stdio.h>
#include <string.h>
#include "sunxi_hal_lradc.h"
#include "sunxi-input.h"
#include "sunxi-keyboard.h"

#ifdef	SUNXIKBD_DEBUG
#define sunxikbd_info(fmt, args...)  printf("%s()%d - "fmt, __func__, __LINE__, ##args)
#else
#define sunxikbd_info(fmt, args...)
#endif

#define sunxikbd_err(fmt, args...)  printf("%s()%d - "fmt, __func__, __LINE__, ##args)

#define ADC_RESOL  64
#define KEY_MAX_CNT 13
#define INITIAL_VALUE 0xff

//key config
struct sunxikbd_config{
	char *name;
	unsigned int measure;
	unsigned int key_num;
	unsigned int scankeycodes[KEY_MAX_CNT];
	unsigned int key_vol[KEY_MAX_CNT];
};
struct sunxikbd_config key_config = {
	.name = "sunxi-keyboard",
	.measure = 1350,
	.key_num = 5,
	.key_vol = {164, 415, 646, 900, 1157},
	.scankeycodes = {115, 114, 248, 164, 116}
};


static unsigned char keypad_mapindex[64] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0,              /* key 1, 0-8 */
	1, 1, 1, 1, 1,                          /* key 2, 9-13 */
	2, 2, 2, 2, 2, 2,                       /* key 3, 14-19 */
	3, 3, 3, 3, 3, 3,                       /* key 4, 20-25 */
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,        /* key 5, 26-36 */
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,        /* key 6, 37-39 */
	6, 6, 6, 6, 6, 6, 6, 6, 6,              /* key 7, 40-49 */
	7, 7, 7, 7, 7, 7, 7                     /* key 8, 50-63 */
};


struct sunxikbd_drv_data{
	struct sunxi_input_dev *input_dev;
	unsigned int scankeycodes[KEY_MAX_CNT];
	unsigned char compare_later;
	unsigned char compare_before;
	unsigned char key_code;
	unsigned char last_key_code;
	unsigned char key_cnt;
};
struct sunxikbd_drv_data *key_data  = NULL;

static void sunxi_report_key_down_event(struct sunxikbd_drv_data *key_data)
{
	if (key_data->last_key_code == INITIAL_VALUE) {
		/* first time report key down event */
		input_report_key(key_data->input_dev,
				key_data->scankeycodes[key_data->key_code], 1);
		input_sync(key_data->input_dev);
		key_data->last_key_code = key_data->key_code;
		return;
	}
	if (key_data->scankeycodes[key_data->key_code] ==
			key_data->scankeycodes[key_data->last_key_code]) {
#ifdef REPORT_REPEAT_KEY_BY_INPUT_CORE
		/* report repeat key down event */
		input_report_key(key_data->input_dev,
				key_data->scankeycodes[key_data->key_code], 1);
		input_sync(key_data->input_dev);
#endif
	} else {
		/* report previous key up event
		 ** and report current key down event
		 **/
		input_report_key(key_data->input_dev,
				key_data->scankeycodes[key_data->last_key_code], 0);
		input_sync(key_data->input_dev);
		input_report_key(key_data->input_dev,
				key_data->scankeycodes[key_data->key_code], 1);
		input_sync(key_data->input_dev);
		key_data->last_key_code = key_data->key_code;
	}
}

void lradc_irq_callback(unsigned int irq_status, unsigned int key_vol)
{
	sunxikbd_info("Key Interrupt\n");
	if (irq_status & LRADC_ADC0_DOWNPEND)
		sunxikbd_info("key down\n");

	if (irq_status & LRADC_ADC0_DATAPEND) {
		sunxikbd_info("key data pend\n");
		key_data->key_cnt++;
		key_data->compare_before = key_vol&0x3f;
		if (key_data->compare_before == key_data->compare_later) {
			key_data->compare_later = key_data->compare_before;
			key_data->key_code = keypad_mapindex[key_vol&0x3f];
			sunxi_report_key_down_event(key_data);
			key_data->key_cnt = 0;
		}
		if (key_data->key_cnt == 2) {
			key_data->compare_later = key_data->compare_before;
			key_data->key_cnt = 0;
		}
	}

	if (irq_status & LRADC_ADC0_UPPEND) {
		input_report_key(key_data->input_dev,
				key_data->scankeycodes[key_data->key_code], 0);
		input_sync(key_data->input_dev);
		sunxikbd_info("key up\n");
		key_data->compare_later = 0;
		key_data->key_cnt = 0;
		key_data->last_key_code = INITIAL_VALUE;
	}
}

static int sunxikbd_data_init(struct sunxikbd_drv_data *key_data, struct sunxikbd_config *sunxikbd_config)
{
	int i, j = 0;
	int key_num = 0;
	unsigned int resol;
	unsigned int key_vol[KEY_MAX_CNT];

	key_num = sunxikbd_config->key_num;
	if (key_num < 1 || key_num > KEY_MAX_CNT)
		return -1;

	resol = sunxikbd_config->measure/ADC_RESOL;

	for (i = 0; i < key_num; i++)
	{
		key_data->scankeycodes[i] = sunxikbd_config->scankeycodes[i];
	}

	for (i = 0; i < key_num; i++)
		key_vol[i] = sunxikbd_config->key_vol[i];
	key_vol[key_num] = sunxikbd_config->measure;

	for (i = 0; i < key_num; i++)
	{
		key_vol[i] += (key_vol[i+1] - key_vol[i])/2;
	}

	for (i = 0; i < 64; i++) {
		if (i * resol > key_vol[j])
			j++;
		keypad_mapindex[i] = j;
	}

	key_data->last_key_code = INITIAL_VALUE;

	return 0;
}

static struct sunxikbd_config *get_keyboard_data()
{
	return &key_config;
}

int sunxi_keyboard_init()
{
	int i, ret = -1;
	struct sunxikbd_config *sunxikbd_config = NULL;
	struct sunxi_input_dev *sunxikbd_dev = NULL;

	key_data = pvPortMalloc(sizeof(struct sunxikbd_drv_data));
	if (NULL == key_data) {
		sunxikbd_err("key data malloc err\n");
		return -1;
	}
	memset(key_data, 0, sizeof(struct sunxikbd_drv_data));

	//get keyboard info
	sunxikbd_config = get_keyboard_data();

	//init key data
	ret = sunxikbd_data_init(key_data, sunxikbd_config);
	if(ret < 0) {
		return -1;
	}

	//input dev init
	sunxikbd_dev = sunxi_input_allocate_device();
	if (NULL == sunxikbd_dev) {
		sunxikbd_err("allocate sunxikbd_dev err\n");
		return -1;
	}
	sunxikbd_dev->name = sunxikbd_config->name;
	for (i = 0; i < sunxikbd_config->key_num; i++)
		input_set_capability(sunxikbd_dev, EV_KEY, key_data->scankeycodes[i]);
	key_data->input_dev = sunxikbd_dev;
	sunxi_input_register_device(key_data->input_dev);

	//init lradc
	hal_lradc_init();
	hal_lradc_register_callback(lradc_irq_callback);

	sunxikbd_info("sunxi-keyboard init success\n");

	return 0;
}
