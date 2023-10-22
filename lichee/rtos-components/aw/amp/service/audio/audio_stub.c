#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

#include "sunxi_amp.h"
#include <hal_cache.h>
#include <hal_mutex.h>
#include <console.h>



MAYBE_STATIC int audio_test1(void)
{
	printf("[%s] line:%d \n", __func__, __LINE__);
	return func_stub(RPCCALL_AUDIO(audio_test1), 1, 0, NULL);
}


MAYBE_STATIC int audio_test2(int arg1, int arg2)
{
	void *args[2] = {0};
	args[0] = (void *)(uintptr_t)arg1;
	args[1] = (void *)(uintptr_t)arg2;

	return func_stub(RPCCALL_AUDIO(audio_test2), 1, ARRAY_SIZE(args), args);
}

MAYBE_STATIC int audio_test3_set(uint8_t *buf, uint32_t len)
{
	void *args[2] = {0};
	args[0] = (void *)buf;
	args[1] = (void *)(uintptr_t)len;

	printf("[%s] line:%d buf=%p, len=%d\n", __func__, __LINE__, buf, len);
	hal_dcache_clean((unsigned long)buf, (unsigned long)len);
	return func_stub(RPCCALL_AUDIO(audio_test3), 1, ARRAY_SIZE(args), args);
}

MAYBE_STATIC int audio_test4_get(uint8_t *buf, uint32_t len)
{
	int ret;
	void *args[2] = {0};
	args[0] = (void *)buf;
	args[1] = (void *)(uintptr_t)len;

	printf("[%s] line:%d buf=%p, len=%d\n", __func__, __LINE__, buf, len);
	hal_dcache_invalidate((unsigned long)buf, (unsigned long)len);
	ret = func_stub(RPCCALL_AUDIO(audio_test4), 1, ARRAY_SIZE(args), args);
	hal_dcache_invalidate((unsigned long)buf, (unsigned long)len);
	return ret;
}

/* AudioTrack */
enum {
	AT_RM_STATE_SETUP = 0,
	AT_RM_STATE_RUNNING,
};

/* AudioRecord */
enum {
	AR_RM_STATE_SETUP = 0,
	AR_RM_STATE_RUNNING,
};

#if defined(CONFIG_AMP_AUDIO_PB_API_UNIQUE)
#include "audio_stub_pb_unique.c"
#elif defined(CONFIG_AMP_AUDIO_PB_API_ALIAS)
#include "audio_stub_pb_alias.c"
#endif

#if defined(CONFIG_AMP_AUDIO_CAP_API_UNIQUE)
#include "audio_stub_cap_unique.c"
#elif defined(CONFIG_AMP_AUDIO_CAP_API_ALIAS)
#include "audio_stub_cap_alias.c"
#endif


#if defined(CONFIG_AMP_AUDIO_UTILS)
#include "audio_stub_utils.c"
#endif
