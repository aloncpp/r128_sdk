#ifndef __ASR_DEMO_CONFIG_H__
#define __ASR_DEMO_CONFIG_H__

//#define CONFIG_ASR_DEMO_NO_PRINT
#define DSP_ASR_DUMP_INPUT_DATA
#define USE_PLAYER_MGR
//path是保存到本地文件，port是通过adb forward保存到PC，如需dump选择一种不为空或0即可
#define SAVE_AUDIO_DATA_PATH		(0) //("/data/ring/audio_data")
#define SAVE_AUDIO_DATA_PORT		(0) //(23334)
#define SAVE_AUDIO_RAW_DATA_PATH	(0) //("/data/ring/audio_raw_data")
#define SAVE_AUDIO_RAW_DATA_PORT	(23333) //(0)

#define AUDIO_RAW_DATA_RING_SIZE	(1000 * 16000 * 2 * 4 / 1000) //1000ms 16bit 16Khz 4ch
#define AUDIO_DATA_RING_SIZE		(1000 * 16000 * 2 / 1000) //1000ms 16bit 16Khz 1ch

#define SOFTAP_AP_CONFIG_PATH		("/data/wifi_config.txt")
#define SOFTAP_AP_SSID				("asr_demo_ap")
#define SOFTAP_AP_PSK				("Aa123456")

#define FRAME_SIZE_TO_MS(size)		((size) / (16000 * 2 / 1000))
#define	DSP_ASR_FRAME_SIZE			(10 * 16000 * 2 / 1000)	//10ms 16bit 16Khz 1ch
#define	DSP_ASR_VAD_BUF_LEN			(700 * 16000 * 2 / 1000) //700ms 16bit 16Khz 1ch
#define	DSP_ASR_TIMEOUT_MS			(15000)

typedef enum {
	DSP_NONE 	= 0,
	DSP_START,
	DSP_STOP,
	DSP_ENABLE_ALG,
	DSP_DUMP_MERGE_DATA,
	DSP_DUMP_RAW_DATA,
	DSP_NUM,
}DSP_CTRL_TYPE;

typedef struct dsp_ctrl_init {
	unsigned int cmd;
} dsp_ctrl_init_t;
// RV to DSP 命令通道
#define RPD_CTL_DIR					(3)
#define RPD_CTL_TYPE				("RVtoDSPCli")
#define RPD_CTL_NAME				("RVtoDSPCliCb")
// 发送dsp命令的数据包格式
#define RPD_CTL_CMD_OFFSET			(0)
#define RPD_CTL_CMD_SIZE			(sizeof(dsp_ctrl_init_t))
#define RPD_CTL_SEND_SIZE			(RPD_CTL_CMD_OFFSET + RPD_CTL_CMD_SIZE)

// DSP to RV 数据通道
#define RPD_DATA_DIR				(3)
#define RPD_DATA_TYPE				("DSPtoRVAsr")
#define RPD_DATA_NAME				("RVrecvDSPsend")
// 接收dsp数据的数据包格式
// 定义DSP_ASR_DUMP_INPUT_DATA时，接收的4ch数据为2mic + 1ref + 1out
// 未定义DSP_ASR_DUMP_INPUT_DATA时，接收的1ch数据为 1out
#define RPD_DATA_VAD_FLAG_OFFSET	(0)
#define RPD_DATA_VAD_FLAG_SIZE		(sizeof(int))
#define RPD_DATA_WORD_ID_OFFSET		(RPD_DATA_VAD_FLAG_OFFSET + RPD_DATA_VAD_FLAG_SIZE)
#define RPD_DATA_WORD_ID_SIZE		(sizeof(int))
#define RPD_DATA_CONFIDENCE_OFFSET	(RPD_DATA_WORD_ID_OFFSET + RPD_DATA_WORD_ID_SIZE)
#define RPD_DATA_CONFIDENCE_SIZE	(sizeof(float))
#define RPD_DATA_CH0_OFFSET			(RPD_DATA_CONFIDENCE_OFFSET + RPD_DATA_CONFIDENCE_SIZE)
#define RPD_DATA_CH0_SIZE			(DSP_ASR_FRAME_SIZE)
#ifdef DSP_ASR_DUMP_INPUT_DATA
#define RPD_DATA_CH1_OFFSET			(RPD_DATA_CH0_OFFSET + RPD_DATA_CH0_SIZE)
#define RPD_DATA_CH1_SIZE			(DSP_ASR_FRAME_SIZE)
#define RPD_DATA_CH2_OFFSET			(RPD_DATA_CH1_OFFSET + RPD_DATA_CH1_SIZE)
#define RPD_DATA_CH2_SIZE			(DSP_ASR_FRAME_SIZE)
#define RPD_DATA_CH3_OFFSET			(RPD_DATA_CH2_OFFSET + RPD_DATA_CH2_SIZE)
#define RPD_DATA_CH3_SIZE			(DSP_ASR_FRAME_SIZE)
#define RPD_DATA_OUTPUT_SIZE		(RPD_DATA_CH3_OFFSET + RPD_DATA_CH3_SIZE)
#define RPD_DATA_RAW_DATA_OFFSET	(RPD_DATA_CH0_OFFSET)
#define RPD_DATA_RAW_DATA_SIZE		(RPD_DATA_CH0_SIZE + RPD_DATA_CH1_SIZE + RPD_DATA_CH2_SIZE + RPD_DATA_CH3_SIZE)
#define RPD_DATA_OUT_DATA_OFFSET	(RPD_DATA_CH3_OFFSET)
#define RPD_DATA_OUT_DATA_SIZE		(RPD_DATA_CH3_SIZE)
#else
#define RPD_DATA_OUTPUT_SIZE		(RPD_DATA_CH0_OFFSET + RPD_DATA_CH0_SIZE)
#define RPD_DATA_RAW_DATA_OFFSET	(RPD_DATA_CH0_OFFSET)
#define RPD_DATA_RAW_DATA_SIZE		(RPD_DATA_CH0_SIZE)
#define RPD_DATA_OUT_DATA_OFFSET	(RPD_DATA_CH0_OFFSET)
#define RPD_DATA_OUT_DATA_SIZE		(RPD_DATA_CH0_SIZE)
#endif

// RV to DSP 数据通道
#define RPD_VAD_DIR					(3)
#define RPD_VAD_TYPE				("RVtoDSPVad")
#define RPD_VAD_NAME				("RVtoDSPVadCb")
// 发送dsp数据的数据包格式
#define RPD_VAD_ENABLE_OFFSET		(0)
#define RPD_VAD_ENABLE_SIZE			(sizeof(unsigned int))
#define RPD_VAD_SEND_SIZE			(RPD_VAD_ENABLE_OFFSET + RPD_VAD_ENABLE_SIZE)

//上传数据：[text:ONLINE_ASR_START_TEXT][bin:音频数据][text:ONLINE_ASR_END_TEXT]
#define	ONLINE_ASR_TIMEOUT_MS		(15000)
#define	ONLINE_ASR_HOST				("your.server.com")
#define	ONLINE_ASR_PATH 			("/your/path")
#define	ONLINE_ASR_PORT				(8000)
#define	ONLINE_ASR_CERT				(NULL)
#define	ONLINE_ASR_START_TEXT		("start")
#define	ONLINE_ASR_END_TEXT			("end")

#endif /* __ASR_DEMO_CONFIG_H__ */
