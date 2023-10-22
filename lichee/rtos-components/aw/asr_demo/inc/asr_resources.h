#ifndef __ASR_RESOURCES_H__
#define __ASR_RESOURCES_H__

// 开关机
#define CMD_ID_POWER_ON_1		(3)
#define CMD_ID_POWER_ON_2		(4)
#define CMD_ID_POWER_ON_3		(5)
#define CMD_ID_POWER_OFF_1		(6)
#define CMD_ID_POWER_OFF_2		(7)
#define CMD_ID_POWER_OFF_3		(8)
//制冷模式 制热模式 除湿模式 送风模式
#define CMD_ID_COOL_MODE_1		(9)
#define CMD_ID_COOL_MODE_2		(10)
#define CMD_ID_HEAT_MODE_1		(11)
#define CMD_ID_HEAT_MODE_2		(12)
#define CMD_ID_DDRY_MODE_1		(13)
#define CMD_ID_DDRY_MODE_2		(14)
#define CMD_ID_FAN_MODE_1		(15)
#define CMD_ID_FAN_MODE_2		(16)
//温度
#define CMD_ID_MIN_TEMP			(16)
#define CMD_ID_MAX_TEMP			(32)
#define CMD_ID_SET_TEMP(t)		(1 + (t))
#define CMD_ID_HIGHER_TEMP_1	(34)
#define CMD_ID_HIGHER_TEMP_2	(35)
#define CMD_ID_HIGHER_TEMP_3	(36)
#define CMD_ID_LOWER_TEMP_1		(37)
#define CMD_ID_LOWER_TEMP_2		(38)
#define CMD_ID_LOWER_TEMP_3		(39)
//风速
#define CMD_ID_FAN_MAX_SPEED_1	(40)
#define CMD_ID_FAN_MAX_SPEED_2	(41)
#define CMD_ID_FAN_MID_SPEED_1	(42)
#define CMD_ID_FAN_MID_SPEED_2	(43)
#define CMD_ID_FAN_MIN_SPEED_1	(44)
#define CMD_ID_FAN_MIN_SPEED_2	(45)
#define CMD_ID_FAN_AUTO_SPEED	(46)
#define CMD_ID_FAN_HIGH_SPEED_1	(47)
#define CMD_ID_FAN_HIGH_SPEED_2	(48)
#define CMD_ID_FAN_LOW_SPEED_1	(49)
#define CMD_ID_FAN_LOW_SPEED_2	(50)
//音量
#define CMD_ID_MAX_VOLUME		(83)
#define CMD_ID_MIN_VOLUME		(84)
#define CMD_ID_HIGHER_VOLUME_1	(85)
#define CMD_ID_HIGHER_VOLUME_2	(86)
#define CMD_ID_LOWER_VOLUME_1	(87)
#define CMD_ID_LOWER_VOLUME_2	(88)
//唤醒
#define CMD_ID_WAKEUP			(117)
#define CMD_ID_TIMEOUT			(118)
//特殊
#define CMD_ID_STATUS_REPORT	(-2)
#define CMD_ID_ASR_RESULT		(-1)

static const char *welcome_url = "/data/asr/welcome.mp3";
static const char *exception_url = "/data/asr/exception.mp3";
static const char *tts_loss_url = "/data/asr/tts_loss.mp3";

const char *get_tts_from_cmd_id(int cmd_id);

#endif /* __ASR_RESOURCES_H__ */