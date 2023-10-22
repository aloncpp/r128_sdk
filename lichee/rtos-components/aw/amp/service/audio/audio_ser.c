#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sunxi_amp.h"
#include <hal_cache.h>

#include <AudioTrack.h>
#include <AudioRecord.h>

#define LOG_COLOR_NONE		"\e[0m"
#define LOG_COLOR_GREEN		"\e[32m"

#define aa_info(fmt, args...)	\
    printf(LOG_COLOR_GREEN "[SERVICE][%s](%d) " fmt "\n" LOG_COLOR_NONE, \
			__func__, __LINE__, ##args)

#define HEXDUMP(ptr, size) \
do { \
	int i; \
	char *p = (char *)ptr; \
	aa_info(""); \
	for (i = 0; i < size; i++) { \
		printf("0x%x ", *(p+i)); \
		if ((i+1)%16 == 0) \
			printf("\n"); \
	} \
	aa_info(""); \
} while (0)

static int audio_test1(void)
{
	aa_info("");
	return 0;
}

static int audio_test2(int arg1, int arg2)
{
	aa_info(" arg1=%d, arg2=%d", arg1, arg2);
	return arg1+arg2;
}

static int audio_test3(uint8_t *buf, uint32_t len)
{
	/* get data from remote, cache align */
	hal_dcache_invalidate((unsigned long)buf, (unsigned long)len);
	aa_info("get data from remote:");
	HEXDUMP(buf, len);
	return 0;
}

static int audio_test4(uint8_t *buf, uint32_t len)
{
	memset(buf, 0x11, len);
	hal_dcache_clean((unsigned long)buf, (unsigned long)len);
	aa_info("set data to remote:");
	return 0;
}

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_TRACK
static void *local_AudioTrackCreate(const char *name, int len)
{
	tAudioTrack *at;

	hal_dcache_invalidate((unsigned long)name, (unsigned long)len);
	at = AudioTrackCreate(name);
	hal_dcache_invalidate((unsigned long)name, (unsigned long)len);

	return (void *)at;
}

static int local_AudioTrackDestroy(tAudioTrack *at)
{
	aa_info("");
	return AudioTrackDestroy(at);
}

static int local_AudioTrackControl(tAudioTrack *at, uint8_t control)
{
	int ret;

	if (control)
		ret = AudioTrackStart(at);
	else
		ret = AudioTrackStop(at);

	return ret;
}

static int local_AudioTrackSetup(tAudioTrack *at, uint32_t rate, uint8_t channels, uint8_t bits)
{
	return AudioTrackSetup(at, rate, channels, bits);
}

static int local_AudioTrackWrite(tAudioTrack *at, void *data, uint32_t size)
{
	int ret;
	hal_dcache_invalidate((unsigned long)data, (unsigned long)size);
	ret = AudioTrackWrite(at, data, size);
	hal_dcache_invalidate((unsigned long)data, (unsigned long)size);
	return ret;
}
#endif

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_RECORD
static void *local_AudioRecordCreate(const char *name, int len)
{
	tAudioRecord *ar;

	hal_dcache_invalidate((unsigned long)name, (unsigned long)len);
	ar = AudioRecordCreate(name);
	hal_dcache_invalidate((unsigned long)name, (unsigned long)len);

	return (void *)ar;
}

static int local_AudioRecordDestroy(tAudioRecord *ar)
{
	aa_info("");
	return AudioRecordDestroy(ar);
}

static int local_AudioRecordControl(tAudioRecord *ar, uint8_t control)
{
	int ret;

	if (control)
		ret = AudioRecordStart(ar);
	else
		ret = AudioRecordStop(ar);

	return ret;
}

static int local_AudioRecordSetup(tAudioRecord *ar, uint32_t rate, uint8_t channels, uint8_t bits)
{
	return AudioRecordSetup(ar, rate, channels, bits);
}

static int local_AudioRecordRead(tAudioRecord *ar, void *data, uint32_t size)
{
	int ret;
	ret = AudioRecordRead(ar, data, size);
	hal_dcache_clean((unsigned long)data, (unsigned long)size);
	return ret;
}
#endif

sunxi_amp_func_table audio_table[] = {
	{.func = (void *)&audio_test1,			.args_num = 0, .return_type = RET_POINTER},
	{.func = (void *)&audio_test2,			.args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&audio_test3,			.args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&audio_test4,			.args_num = 2, .return_type = RET_POINTER},
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_TRACK
	/* AudioTrack */
	{.func = (void *)&local_AudioTrackCreate,	.args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&local_AudioTrackDestroy,	.args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&local_AudioTrackControl,	.args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&local_AudioTrackSetup,	.args_num = 4, .return_type = RET_POINTER},
	{.func = (void *)&local_AudioTrackWrite,	.args_num = 3, .return_type = RET_POINTER},
#else
	{.func = (void *)NULL,				.args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)NULL,				.args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)NULL,				.args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)NULL,				.args_num = 4, .return_type = RET_POINTER},
	{.func = (void *)NULL,				.args_num = 3, .return_type = RET_POINTER},
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_RECORD
	/* AudioRecord */
	{.func = (void *)&local_AudioRecordCreate,	.args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&local_AudioRecordDestroy,	.args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&local_AudioRecordControl,	.args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&local_AudioRecordSetup,	.args_num = 4, .return_type = RET_POINTER},
	{.func = (void *)&local_AudioRecordRead,	.args_num = 3, .return_type = RET_POINTER},
#else
	{.func = (void *)NULL,				.args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)NULL,				.args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)NULL,				.args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)NULL,				.args_num = 4, .return_type = RET_POINTER},
	{.func = (void *)NULL,				.args_num = 3, .return_type = RET_POINTER},
#endif
};
