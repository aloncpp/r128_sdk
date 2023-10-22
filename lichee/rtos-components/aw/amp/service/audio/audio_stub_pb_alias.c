#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

#include "sunxi_amp.h"
#include <hal_cache.h>
#include <hal_mutex.h>
#include <console.h>

#include "include/audio_amp.h"

struct AudioTrackRM {
	void *at;
	hal_mutex_t mutex;
	uint8_t state;
};

tAudioTrackRM *AudioTrackCreateRM(const char *name)
{
	void *at;
	tAudioTrackRM *atrm;
	void *args[2] = {0};
	int len;
	char *align_name;

	len = strlen(name) + 1;
	align_name = amp_align_malloc(len);
	if (!align_name)
		return NULL;

	memset(align_name, 0, len);
	memcpy(align_name, name, len);
	args[0] = (void *)align_name;
	args[1] = (void *)(uintptr_t)len;

	hal_dcache_clean((unsigned long)align_name, (unsigned long)len);
	at = (void *)func_stub(RPCCALL_AUDIO(AudioTrackCreateRM), 1, ARRAY_SIZE(args), args);
	amp_align_free(align_name);

	if (!at)
		return NULL;
	atrm = malloc(sizeof(tAudioTrackRM));
	if (!atrm)
		goto err;
	atrm->at = at;

	atrm->mutex = hal_mutex_create();

	atrm->state = AT_RM_STATE_SETUP;

	return atrm;
err:
	if (atrm)
		free(atrm);
	return NULL;
}

int AudioTrackDestroyRM(tAudioTrackRM *atrm)
{
	int ret;
	void *args[1] = {0};

	printf("[%s] line:%d \n", __func__, __LINE__);
	hal_mutex_lock(atrm->mutex);
	args[0] = (void *)atrm->at;
	ret = (int)func_stub(RPCCALL_AUDIO(AudioTrackDestroyRM), 1, ARRAY_SIZE(args), args);
	printf("[%s] line:%d ret=%d\n", __func__, __LINE__, ret);

	hal_mutex_unlock(atrm->mutex);
	hal_mutex_delete(atrm->mutex);

	free(atrm);

	printf("[%s] line:%d \n", __func__, __LINE__);
	return ret;
}

static int AudioTrackControlRM(tAudioTrackRM *atrm, uint32_t control)
{
	int ret;
	void *args[2] = {0};

	args[0] = (void *)atrm->at;
	args[1] = (void *)(uintptr_t)control;

	hal_mutex_lock(atrm->mutex);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioTrackControlRM), 1, ARRAY_SIZE(args), args);
	hal_mutex_unlock(atrm->mutex);

	return ret;
}

int AudioTrackStartRM(tAudioTrackRM *atrm)
{
	return AudioTrackControlRM(atrm, 1);
}

int AudioTrackStopRM(tAudioTrackRM *atrm)
{
	return AudioTrackControlRM(atrm, 0);
}

int AudioTrackSetupRM(tAudioTrackRM *atrm, uint32_t rate, uint8_t channels, uint8_t bits)
{
	int ret;
	void *args[4] = {0};

	args[0] = (void *)atrm->at;
	args[1] = (void *)(uintptr_t)rate;
	args[2] = (void *)(uintptr_t)channels;
	args[3] = (void *)(uintptr_t)bits;

	hal_mutex_lock(atrm->mutex);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioTrackSetupRM), 1, ARRAY_SIZE(args), args);
	hal_mutex_unlock(atrm->mutex);

	return ret;
}

int AudioTrackWriteRM(tAudioTrackRM *atrm, void *data, uint32_t size)
{
	int ret;
	void *args[3] = {0};

	args[0] = (void *)atrm->at;
	args[1] = (void *)data;
	args[2] = (void *)(uintptr_t)size;

	hal_mutex_lock(atrm->mutex);
	hal_dcache_clean((unsigned long)data, (unsigned long)size);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioTrackWriteRM), 1, ARRAY_SIZE(args), args);
	hal_mutex_unlock(atrm->mutex);

	return ret;
}
