#include "asr_resources.h"

struct offline_tts_t {
	int cmd_id;
	const char *tts; // response tts
};

static struct offline_tts_t offline_tts[] = {
	{CMD_ID_POWER_ON_1,			"/data/asr/003.mp3"},
	{CMD_ID_POWER_ON_2,			"/data/asr/003.mp3"},
	{CMD_ID_POWER_ON_3,			"/data/asr/003.mp3"},
	{CMD_ID_POWER_OFF_1,		"/data/asr/006.mp3"},
	{CMD_ID_POWER_OFF_2,		"/data/asr/006.mp3"},
	{CMD_ID_POWER_OFF_3,		"/data/asr/006.mp3"},
	{CMD_ID_COOL_MODE_1,		"/data/asr/009.mp3"},
	{CMD_ID_COOL_MODE_2,		"/data/asr/009.mp3"},
	{CMD_ID_HEAT_MODE_1,		"/data/asr/011.mp3"},
	{CMD_ID_HEAT_MODE_2,		"/data/asr/011.mp3"},
	{CMD_ID_DDRY_MODE_1,		"/data/asr/013.mp3"},
	{CMD_ID_DDRY_MODE_2,		"/data/asr/013.mp3"},
	{CMD_ID_FAN_MODE_1,			"/data/asr/015.mp3"},
	{CMD_ID_FAN_MODE_2,			"/data/asr/015.mp3"},
	{CMD_ID_SET_TEMP(16),		"/data/asr/017.mp3"},
	{CMD_ID_SET_TEMP(17),		"/data/asr/018.mp3"},
	{CMD_ID_SET_TEMP(18),		"/data/asr/019.mp3"},
	{CMD_ID_SET_TEMP(19),		"/data/asr/020.mp3"},
	{CMD_ID_SET_TEMP(20),		"/data/asr/021.mp3"},
	{CMD_ID_SET_TEMP(21),		"/data/asr/022.mp3"},
	{CMD_ID_SET_TEMP(22),		"/data/asr/023.mp3"},
	{CMD_ID_SET_TEMP(23),		"/data/asr/024.mp3"},
	{CMD_ID_SET_TEMP(24),		"/data/asr/025.mp3"},
	{CMD_ID_SET_TEMP(25),		"/data/asr/026.mp3"},
	{CMD_ID_SET_TEMP(26),		"/data/asr/027.mp3"},
	{CMD_ID_SET_TEMP(27),		"/data/asr/028.mp3"},
	{CMD_ID_SET_TEMP(28),		"/data/asr/029.mp3"},
	{CMD_ID_SET_TEMP(29),		"/data/asr/030.mp3"},
	{CMD_ID_SET_TEMP(30),		"/data/asr/031.mp3"},
	{CMD_ID_SET_TEMP(31),		"/data/asr/032.mp3"},
	{CMD_ID_SET_TEMP(32),		"/data/asr/033.mp3"},
	{CMD_ID_HIGHER_TEMP_1,		"/data/asr/035.mp3"},
	{CMD_ID_HIGHER_TEMP_2,		"/data/asr/035.mp3"},
	{CMD_ID_HIGHER_TEMP_3,		"/data/asr/035.mp3"},
	{CMD_ID_LOWER_TEMP_1,		"/data/asr/038.mp3"},
	{CMD_ID_LOWER_TEMP_2,		"/data/asr/038.mp3"},
	{CMD_ID_LOWER_TEMP_3,		"/data/asr/038.mp3"},
	{CMD_ID_FAN_MAX_SPEED_1,	"/data/asr/041.mp3"},
	{CMD_ID_FAN_MAX_SPEED_2,	"/data/asr/041.mp3"},
	{CMD_ID_FAN_MID_SPEED_1,	"/data/asr/043.mp3"},
	{CMD_ID_FAN_MID_SPEED_2,	"/data/asr/043.mp3"},
	{CMD_ID_FAN_MIN_SPEED_1,	"/data/asr/045.mp3"},
	{CMD_ID_FAN_MIN_SPEED_2,	"/data/asr/045.mp3"},
	{CMD_ID_FAN_HIGH_SPEED_1,	"/data/asr/047.mp3"},
	{CMD_ID_FAN_HIGH_SPEED_2,	"/data/asr/047.mp3"},
	{CMD_ID_FAN_LOW_SPEED_1,	"/data/asr/049.mp3"},
	{CMD_ID_FAN_LOW_SPEED_2,	"/data/asr/049.mp3"},
	{CMD_ID_MAX_VOLUME,			"/data/asr/max_volume.mp3"},
	{CMD_ID_MIN_VOLUME,			"/data/asr/min_volume.mp3"},
	{CMD_ID_HIGHER_VOLUME_1,	"/data/asr/higher_volume.mp3"},
	{CMD_ID_HIGHER_VOLUME_2,	"/data/asr/higher_volume.mp3"},
	{CMD_ID_LOWER_VOLUME_1,		"/data/asr/lower_volume.mp3"},
	{CMD_ID_LOWER_VOLUME_2,		"/data/asr/lower_volume.mp3"},
	{CMD_ID_WAKEUP,				"/data/asr/hello.mp3"},
	{CMD_ID_TIMEOUT,			"/data/asr/offline_timeout.mp3"},
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

const char *get_tts_from_cmd_id(int cmd_id)
{
	for(int i = 0; i < ARRAY_SIZE(offline_tts); i++) {
		if(offline_tts[i].cmd_id == cmd_id)
			return offline_tts[i].tts;
	}

	return tts_loss_url;
}
