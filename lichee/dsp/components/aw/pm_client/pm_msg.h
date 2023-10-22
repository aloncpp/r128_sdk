
#ifndef _PM_DSP_SUSPEND_H_
#define _PM_DSP_SUSPEND_H_

typedef struct pm_msg {
	uint32_t inited;
	struct msg_endpoint    *edp;
	QueueHandle_t		xQueue;
	SemaphoreHandle_t	xSemaphore;
	TaskHandle_t            xHandle;
} pm_msg_t;

enum msgbox_channel_direction {
	MSGBOX_CHANNEL_RECEIVE,
	MSGBOX_CHANNEL_SEND,
};

#if defined(CONFIG_ARCH_SUN8IW20)
#define MSGBOX_PM_REMOTE       0
#define MSGBOX_PM_RECV_CHANNEL 0
#define MSGBOX_PM_SEND_CHANNEL 0
#elif defined(CONFIG_ARCH_SUN20IW2)
#define MSGBOX_PM_REMOTE       2
#define MSGBOX_PM_RECV_CHANNEL 0
#define MSGBOX_PM_SEND_CHANNEL 0
#else
#error "Only support R528/D1."
#endif

#define  PM_DSP_POWER_SUSPEND 0xf3f30102
#define  PM_DSP_POWER_RESUME  0xf3f30201

#define MSGBOX_PM_XQUEUE_LENTH 8

#ifdef CONFIG_AW_DSPFREQ
extern int dsp_set_freq(int clk_rate);
extern int dsp_get_freq(void);
#endif

int pm_standby_service_init(void);

#endif

