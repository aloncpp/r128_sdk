/*
 * airkiss.h
 *
 *  Created on: 2015-1-26
 *      Author: peterfan
 */

#ifndef _AIRKISS_H_
#define _AIRKISS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wlan_airkiss.h"

/*
 * ����AIRKISS_ENABLE_CRYPTΪ1������AirKiss���ܹ���
 */
#ifndef AIRKISS_ENABLE_CRYPT
#define AIRKISS_ENABLE_CRYPT 1
#endif

enum loglevel{
	OFF = 0,
	ERROR = 1,
	INFO = 2,
};

#define AIRKISS_DBG(level, fmt, args...) \
	do {		\
		if (level <= g_debuglevel)	\
			printf("[airkiss]"fmt,##args);	\
	} while (0)


#define AIRKISS_BUG_ON(v) do {if(v) {printf("BUG at %s:%d!\n", __func__, __LINE__); \
			      while (1);}} while (0)

typedef void* (*airkiss_memset_fn) (void* ptr, int value, unsigned int num);
typedef void* (*airkiss_memcpy_fn) (void* dst, const void* src, unsigned int num);
typedef int (*airkiss_memcmp_fn) (const void* ptr1, const void* ptr2, unsigned int num);
typedef int (*airkiss_printf_fn) (const char* format, ...);

/*
 * ��AirKiss��������ã�Ŀǰ��������һЩ�ص�����
 */
typedef struct
{
	/*
	 * Ϊ�������ٿ��ļ����������c��׼�⺯����Ҫ�ϲ�ʹ�����ṩ
	 * ����printf����ΪNULL
	 */
	airkiss_memset_fn memset;
	airkiss_memcpy_fn memcpy;
	airkiss_memcmp_fn memcmp;
	airkiss_printf_fn printf;

} airkiss_config_t;



/*
 * AirKiss API������Ҫ�Ľṹ�壬����Ϊȫ�ֱ�������ͨ��malloc��̬����
 */
typedef struct
{
	int dummyap[26];
	int dummy[32];
} airkiss_context_t;



/*
 * AirKiss����ɹ���Ľ��
 */
typedef struct
{
	char* pwd;		/* wifi���룬��'\0'��β */
	char* ssid;		/* wifi ssid����'\0'��β */
	unsigned char pwd_len;	/* wifi���볤�� */
	unsigned char ssid_len;	/* wifi ssid���� */
	unsigned char random;	/* ���ֵ������AirKissЭ�飬��wifi���ӳɹ�����Ҫͨ��udp��10000�˿ڹ㲥������ֵ������AirKiss���Ͷˣ�΢�ſͻ��˻���AirKissDebugger������֪��AirKiss�����óɹ� */
	unsigned char reserved;	/* ����ֵ */
} airkiss_result_t;


#if AIRKISS_ENABLE_CRYPT

/*
 * ���ý���key�������Ϊ128bit���������key����128bit����Ĭ����0���
 *
 * ����ֵ
 * 		< 0������ͨ���ǲ�������
 * 		  0���ɹ�
 */
int airkiss_set_key(airkiss_context_t* context, const unsigned char* key, unsigned int length);

#endif



/*
 * ��ȡAirKiss��汾��Ϣ
 */
const char* airkiss_version(void);



/*
 * ��ʼ��AirKiss�⣬��Ҫ����context�����Զ�ε���
 *
 * ����ֵ
 * 		< 0������ͨ���ǲ�������
 * 		  0���ɹ�
 */
int airkiss_init(airkiss_context_t* context, const airkiss_config_t* config);



/*
 * ����WiFi Promiscuous Mode�󣬽��յ��İ�����airkiss_recv�Խ��н���
 *
 * ����˵��
 * 		frame��802.11 frame mac header(must contain at least first 24 bytes)
 * 		length��total frame length
 *
 * ����ֵ
 * 		 < 0������ͨ���ǲ�������
 * 		>= 0���ɹ�����ο�airkiss_status_t
 */
int airkiss_recv(airkiss_context_t* context, const void* frame, unsigned short length);



/*
 * ��airkiss_recv()����AIRKISS_STATUS_COMPLETE�󣬵��ô˺�������ȡAirKiss������
 *
 * ����ֵ
 * 		< 0����������״̬������AIRKISS_STATUS_COMPLETE
 * 		  0���ɹ�
 */
int airkiss_get_result(airkiss_context_t* context, airkiss_result_t* result);


/*
 * �ϲ��л��ŵ��Ժ󣬿��Ե���һ�±��ӿ��建�棬�����������ŵ��ĸ��ʣ�ע����õ��߼�����airkiss_init֮��
 *
 * ����ֵ
 * 		< 0������ͨ���ǲ�������
 * 		  0���ɹ�
 */
int airkiss_change_channel(airkiss_context_t* context);

/*
 *
 * ������ʵ������������������API��������΢�������������API
 *
 */

/*
 * airkiss_lan_recv()�ķ���ֵ
 */
typedef enum
{
	/* �ṩ�����ݻ��������Ȳ��� */
	AIRKISS_LAN_ERR_OVERFLOW = -5,

	/* ��ǰ�汾��֧�ֵ�ָ������ */
	AIRKISS_LAN_ERR_CMD = -4,

	/* ������ݳ��� */
	AIRKISS_LAN_ERR_PAKE = -3,

	/* �������ݲ������� */
	AIRKISS_LAN_ERR_PARA = -2,

	/* �������ݴ��� */
	AIRKISS_LAN_ERR_PKG = -1,

	/* ���ĸ�ʽ��ȷ�����ǲ���Ҫ�豸��������ݰ� */
	AIRKISS_LAN_CONTINUE = 0,

	/* ���յ������豸�������ݰ� */
	AIRKISS_LAN_SSDP_REQ = 1,

	/* ���ݰ������� */
	AIRKISS_LAN_PAKE_READY = 2


} airkiss_lan_ret_t;


typedef enum
{
	AIRKISS_LAN_SSDP_REQ_CMD = 0x1,
	AIRKISS_LAN_SSDP_RESP_CMD = 0x1001,
	AIRKISS_LAN_SSDP_NOTIFY_CMD = 0x1002
} airkiss_lan_cmdid_t;

/*
 * �豸������������ģʽ�󣬽��յ��İ�����airkiss_lan_recv�Խ��н���
 *
 * ����˵��
 * 		body��802.11 frame mac header(must contain at least first 24 bytes)
 * 		length��total frame length
 * 		config��AirKiss�ص�����
 *
 * ����ֵ
 * 		 < 0��������ο�airkiss_lan_ret_t��ͨ���Ǳ������ݳ���
 * 		>= 0���ɹ�����ο�airkiss_lan_ret_t
 */
int airkiss_lan_recv(const void* body, unsigned short length, const airkiss_config_t* config);

/*
 * �豸Ҫ��������Э���ʱ�����ñ��ӿ�������ݰ����
 *
 * ����˵��
 * 		body��802.11 frame mac header(must contain at least first 24 bytes)
 * 		length��total frame length
 * 		config��AirKiss�ص�����
 *
 * ����ֵ
 * 		 < 0��������ο�airkiss_lan_ret_t��ͨ���Ǳ������ݳ���
 * 		>= 0���ɹ�����ο�airkiss_lan_ret_t
 */
int airkiss_lan_pack(airkiss_lan_cmdid_t ak_lan_cmdid, void* appid, void* deviceid, void* _datain, unsigned short inlength, void* _dataout, unsigned short* outlength, const airkiss_config_t* config);

typedef struct {
	char *app_id;
	char *device_id;
	uint32_t ack_period_ms;
} Airkiss_Online_Ack_Info;

#define AK_KEY_LEN 16
#define CHTABLESIZE 13
#define MAX_TIME 4294967295
#define AIRKISS_CYCLE_ACK_THREAD_STACK_SIZE	(1024 * 1)
#define AIRKISS_ONLINE_DIALOG_THREAD_STACK_SIZE	(1024 * 1)

#define AIRKISS_ACK_UDP_PORT 10000
#define AIRKISS_LAN_BUF_LEN 200

#define AK_ACK_TIME_OUT_MS 3000

#define AK_TASK_RUN     (1 << 0)
#define AK_TASK_STOP    (1 << 1)

typedef struct airkiss_priv {
	struct netif *nif;
	airkiss_context_t context;
	airkiss_result_t result;
	airkiss_status_t status;
#if AIRKISS_ENABLE_CRYPT
	char aes_key[AK_KEY_LEN];
#endif
	airkiss_config_t func;
	uint8_t waiting;
	uint8_t ack_run;
} airkiss_priv_t;

int airkiss_ack_start(airkiss_priv_t *priv, uint32_t random_num, uint32_t timeout_ms);
int airkiss_ack_stop(airkiss_priv_t *priv);

#ifdef __cplusplus
}
#endif

#endif /* _AIRKISS_H_ */
