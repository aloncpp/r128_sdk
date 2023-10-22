/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __AUDIOBASE_H
#define __AUDIOBASE_H

#include <assert.h>
#include <aw_common.h>
#include <aw_list.h>
#include <errno.h>

#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/common.h>

#include <hal_time.h>
#include <hal_sem.h>
#include <hal_mutex.h>
#include <hal_status.h>

#define AUDIO_SYSTEM_VERSION "AW-V0.1.12"

//#define RB_DEBUG

#ifndef TAG
#define TAG	"AS"
#endif

#ifndef unlikely
#define unlikely(x)             __builtin_expect ((x), 0)
#endif

#define LOG_COLOR_NONE		"\e[0m"
#define LOG_COLOR_GREEN		"\e[32m"
#define LOG_COLOR_BLUE		"\e[34m"
#define LOG_COLOR_RED		"\e[31m"

#if 1
#define as_alloc(size)		calloc(1, size)
#define as_free(ptr)		free(ptr)
#else
static inline void *as_alloc(size)
{
	printf("alloc %u bytes\n", size);
	return calloc(1, size);
}

static inline void as_free(void *ptr)
{
	free(ptr);
}
#endif

#if defined(CONFIG_ARCH_ARM_CORTEX_A7)
#define AS_CORE "A7-"
#elif defined(CONFIG_ARCH_ARM_CORTEX_M33)
#define AS_CORE "M33-"
#elif defined(CONFIG_ARCH_RISCV)
#define AS_CORE "RV-"
#elif defined(CONFIG_ARCH_DSP)
#define AS_CORE "DSP-"
#else
#define AS_CORE ""
#endif

extern int g_as_debug_mask;
#define _debug(fmt, args...) \
do { \
	if (unlikely(g_as_debug_mask)) \
		printf(LOG_COLOR_GREEN "[%s%s-DBG][%s](%d) " fmt "\n" \
			LOG_COLOR_NONE, AS_CORE, TAG, __func__, __LINE__, ##args); \
} while (0)


#define _info(fmt, args...)	\
    printf(LOG_COLOR_BLUE "[%s%s-INF][%s](%d) " fmt "\n" LOG_COLOR_NONE, \
			AS_CORE, TAG, __func__, __LINE__, ##args)

#define _err(fmt, args...)	\
    printf(LOG_COLOR_RED "[%s%s-ERR][%s](%d) " fmt "\n" LOG_COLOR_NONE, \
			AS_CORE, TAG, __func__, __LINE__, ##args)

#if 1
#define fatal(msg) \
do {\
	printf("[%s%s-FATAL][%s](%d) %s \n", AS_CORE, TAG, __func__, __LINE__,msg);\
	assert(0);\
} while (0)
#else
#define fatal(msg) \
	assert(0);
#endif

#ifndef configAPPLICATION_AUDIO_PRIORITY
#define configAPPLICATION_AUDIO_PRIORITY	(configMAX_PRIORITIES > 20 ? configMAX_PRIORITIES - 8 : configMAX_PRIORITIES - 3)
#endif
#define configAPPLICATION_AUDIO_HIGH_PRIORITY	(configAPPLICATION_AUDIO_PRIORITY + 2)

#define AS_MAYBE_UNUSED(v) 	(void)(v)

struct rb_attr {
	volatile snd_pcm_uframes_t hw_ptr;
	snd_pcm_uframes_t appl_ptr;
	void *rb_buf;
	uint32_t rb_bytes;
};

typedef hal_sem_t as_sem_t;
#define as_sem_create(cnt) 	 	hal_sem_create(cnt)
#define as_sem_delete(sem) 	 	hal_sem_delete(sem)
#define as_sem_post(sem) 	 	hal_sem_post(sem)
#define as_sem_timedwait(sem, ticks) 	hal_sem_timedwait(sem, ticks)

typedef hal_mutex_t as_mutex_t;
#define as_mutex_init() 	hal_mutex_create()
#define as_mutex_lock(mutex) 	hal_mutex_lock(mutex)
#define as_mutex_unlock(mutex) 	hal_mutex_unlock(mutex)
#define as_mutex_destroy(mutex)	hal_mutex_delete(mutex)


typedef struct as_schd as_schd_t;
as_schd_t *as_schd_init(void);
int as_schd_timeout(as_schd_t *schd, long ms);
void as_schd_wakeup(as_schd_t *schd);
void as_schd_destroy(as_schd_t *schd);

typedef struct as_pcm_config {
	uint32_t rate;
	uint32_t period_frames;
	uint32_t buffer_frames;
	snd_pcm_uframes_t boundary;
	uint8_t periods;
	snd_pcm_format_t format;
	uint8_t channels;
	uint8_t frame_bytes;
} as_pcm_config_t;

#define as_msleep(ms) 	hal_msleep(ms)

static inline int format_to_bits(snd_pcm_format_t format)
{
	int bits = -1;
	switch (format) {
	case SND_PCM_FORMAT_S16_LE:
		bits = 16;
		break;
	case SND_PCM_FORMAT_S32_LE:
		bits = 32;
		break;
	default:
		printf("unsupported format:%d\n", format);
		break;
	}
	return bits;
}

#endif /* __AUDIOBASE_H */
