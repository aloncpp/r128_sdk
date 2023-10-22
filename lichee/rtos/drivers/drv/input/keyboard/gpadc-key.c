#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <stdio.h>
#include <string.h>
#include "sunxi_hal_gpadc.h"
#include "sunxi-input.h"
#include "gpadc-key.h"

//#define SUNXIKBD_DEBUG
#ifdef	SUNXIKBD_DEBUG
#define sunxikbd_info(fmt, args...)  printf("%s()%d - "fmt, __func__, __LINE__, ##args)
#else
#define sunxikbd_info(fmt, args...)
#endif

#define sunxikbd_err(fmt, args...)  printf("%s()%d - "fmt, __func__, __LINE__, ##args)

#define ADC_RESOL  64
#define KEY_MAX_CNT 13
#define INITIAL_VALUE 0xff

#if defined(CONFIG_ARCH_SUN20IW2P1)
#define ADC_CHANNEL			GP_CH_2
#define MAXIMUM_INPUT_VOLTAGE           2500
#define VOL_RANGE			(2500000UL)
uint8_t filter_cnt = 150;
#else
#define ADC_CHANNEL			GP_CH_0
#define MAXIMUM_INPUT_VOLTAGE           1800
#define VOL_RANGE			(1800000UL)
uint8_t filter_cnt = 5;
#endif

#define DEVIATION			100
#define SUNXIKEY_DOWN			(MAXIMUM_INPUT_VOLTAGE-DEVIATION)
#define SUNXIKEY_UP			SUNXIKEY_DOWN

#define MAXIMUM_SCALE                   128
#define SCALE_UNIT                      (MAXIMUM_INPUT_VOLTAGE/MAXIMUM_SCALE)

//key config
struct sunxikbd_config{
	unsigned int measure;
	char *name;
	unsigned int key_num;
	unsigned int scankeycodes[KEY_MAX_CNT];
	unsigned int key_vol[KEY_MAX_CNT];
};

#if defined(CONFIG_ARCH_SUN20IW2P1)
static struct sunxikbd_config key_config = {
	.measure = 2500,
	.name = "gpadc-key",
	.key_num = 5,
	.key_vol = {164,415,646,900,1157},
	.scankeycodes = {115,114,139,164,116}
};
#else
static struct sunxikbd_config key_config = {
	.measure = 1800,
	.name = "gpadc-key",
	.key_num = 5,
	.key_vol = {164,415,646,900,1157},
	.scankeycodes = {115,114,139,28,102}
};
#endif

static unsigned char keypad_mapindex[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0,              /* key 1, 0-8 */
	1, 1, 1, 1, 1,                          /* key 2, 9-13 */
	2, 2, 2, 2, 2, 2,                       /* key 3, 14-19 */
	3, 3, 3, 3, 3, 3,                       /* key 4, 20-25 */
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,        /* key 5, 26-36 */
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,        /* key 6, 37-39 */
	6, 6, 6, 6, 6, 6, 6, 6, 6,              /* key 7, 40-49 */
	7, 7, 7, 7, 7, 7, 7                     /* key 8, 50-63 */
};


struct sunxi_gpadc_key {
	struct sunxi_input_dev *input_dev;
	unsigned int scankeycodes[KEY_MAX_CNT];
	unsigned char compare_later;
	unsigned char compare_before;
	unsigned char key_code;
	unsigned char last_key_code;
	unsigned char key_cnt;
};
static struct sunxi_gpadc_key *key_data  = NULL;
struct sunxi_input_dev *sunxikbd_dev = NULL;

int gpadc_irq_callback(uint32_t data_type, uint32_t data)
{
	uint32_t vol_data;
	uint8_t ch_num;

	if (data_type == GPADC_UP) {
		key_data->compare_later = 0;
		key_data->key_cnt = 0;
		input_report_key(sunxikbd_dev, key_data->scankeycodes[key_data->key_code], 0);
		input_sync(sunxikbd_dev);
		gpadc_key_disable_highirq(ADC_CHANNEL);
#ifdef CONFIG_COMPONENTS_PM
		hal_gpadc_report_wakeup_event();
#endif
	}
	data = ((VOL_RANGE / 4096) * data); /* 12bits sample rate */
	vol_data = data / 1000;

	if (vol_data < SUNXIKEY_DOWN) {
		/* MAX compare_before = 128 */
		key_data->compare_before = ((data / SCALE_UNIT) / 1000) & 0xff;;
		if (key_data->compare_before >= key_data->compare_later - 1
			&& key_data->compare_before <= key_data->compare_later + 1)
			key_data->key_cnt++;
		else
			key_data->key_cnt = 0;

		key_data->compare_later = key_data->compare_before;
		if (key_data->key_cnt >= filter_cnt) {
			key_data->compare_later = key_data->compare_before;
			key_data->key_code = keypad_mapindex[key_data->compare_before];
			key_data->compare_later = 0;
			key_data->key_cnt = 0;
			if (key_data->key_code  < key_config.key_num) {
				input_report_key(sunxikbd_dev, key_data->scankeycodes[key_data->key_code], 1);
				input_sync(sunxikbd_dev);
				gpadc_key_enable_highirq(ADC_CHANNEL);
#ifdef CONFIG_COMPONENTS_PM
				hal_gpadc_report_wakeup_event();
#endif
			}
		}
	}

	return 0;

}

static int sunxikbd_data_init(struct sunxi_gpadc_key *key_data, struct sunxikbd_config *sunxikbd_config)
{
	int i, j = 0;
	int key_num = 0;
	unsigned int resol;
	unsigned int key_vol[KEY_MAX_CNT];

	key_num = sunxikbd_config->key_num;
	if (key_num < 1 || key_num > KEY_MAX_CNT)
		return -1;

	resol = sunxikbd_config->measure / MAXIMUM_SCALE;

	for (i = 0; i < key_num; i++)
		key_data->scankeycodes[i] = sunxikbd_config->scankeycodes[i];

	for (i = 0; i < key_num; i++)
		key_vol[i] = sunxikbd_config->key_vol[i];

	for (i = 0; i < (key_num - 1); i++)
		key_vol[i] += (key_vol[i + 1] - key_vol[i]) / 2;


	for (i = 0; i < MAXIMUM_SCALE; i++)
	{
		if (i * resol > key_vol[j])
			j++;
		keypad_mapindex[i] = j;
	}

	key_data->last_key_code = INITIAL_VALUE;

	return 0;

}

static struct sunxikbd_config *get_keyboard_data(void)
{
	return &key_config;
}

int sunxi_gpadc_key_init(void)
{
	int i, ret = -1;
	struct sunxikbd_config *sunxikbd_config = NULL;

	key_data = pvPortMalloc(sizeof(struct sunxi_gpadc_key));
	if (NULL == key_data) {
		sunxikbd_err("key data malloc err\n");
		return -1;
	} else
		sunxikbd_info("key data malloc ok!\n");

	memset(key_data, 0, sizeof(struct sunxi_gpadc_key));

	//get keyboard info
	sunxikbd_config = get_keyboard_data();

	//init key data
	ret = sunxikbd_data_init(key_data, sunxikbd_config);
	if(ret < 0)
		return -1;
	else
		sunxikbd_info("sunxikbd_data_init ok !\n");

	//input dev init
	sunxikbd_dev = sunxi_input_allocate_device();
	if (NULL == sunxikbd_dev) {
		sunxikbd_err("allocate sunxikbd_dev err\n");
		return -1;
	} else
		sunxikbd_info("allocate sunxikbd_dev ok! \n");

	sunxikbd_dev->name = sunxikbd_config->name;
	for (i = 0; i < sunxikbd_config->key_num; i++)
		input_set_capability(sunxikbd_dev, EV_KEY, key_data->scankeycodes[i]);

	key_data->input_dev = sunxikbd_dev;
	ret = sunxi_input_register_device(sunxikbd_dev);

	if (ret < 0)
		sunxikbd_err("sunxi_input_register_device err!.\n");
	else
		sunxikbd_info("sunxi_input_register_device ok! \n");

	//init lradc
	ret = hal_gpadc_init();
	if (ret < 0)
		sunxikbd_err("hal gpadc init err!\n");
	else
		sunxikbd_info("hal gpadc init ok !\n");

	hal_gpadc_channel_init(ADC_CHANNEL);
	ret = hal_gpadc_register_callback(ADC_CHANNEL, gpadc_irq_callback);
	if (ret < 0)
		sunxikbd_err("hal_gpadc_register_callback err!\n");
	else
		sunxikbd_info("hal_gpadc_register_callback ok!\n");

	sunxikbd_info("sunxi-gpadc-key init success===========\n");

	return 0;
}
