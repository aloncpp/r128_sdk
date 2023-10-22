#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

#include "sunxi_amp.h"
#include <hal_cache.h>
#include <hal_mutex.h>
#include <console.h>

#include "include/audio_amp.h"

struct AudioRecordRM{
	void *ar;
	hal_mutex_t mutex;
	uint8_t state;
};

tAudioRecordRM *AudioRecordCreateRM(const char *name)
{
	void *ar;
	tAudioRecordRM *arrm;
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
	ar = (void *)func_stub(RPCCALL_AUDIO(AudioRecordCreateRM), 1, ARRAY_SIZE(args), args);
	amp_align_free(align_name);

	if (!ar)
		return NULL;
	arrm = malloc(sizeof(tAudioRecordRM));
	if (!arrm)
		goto err;
	arrm->ar = ar;

	arrm->mutex = hal_mutex_create();

	arrm->state = AT_RM_STATE_SETUP;

	return arrm;
err:
	if (arrm)
		free(arrm);
	return NULL;
}

int AudioRecordDestroyRM(tAudioRecordRM *arrm)
{
	int ret;
	void *args[1] = {0};

	printf("[%s] line:%d \n", __func__, __LINE__);
	hal_mutex_lock(arrm->mutex);
	args[0] = (void *)arrm->ar;
	ret = (int)func_stub(RPCCALL_AUDIO(AudioRecordDestroyRM), 1, ARRAY_SIZE(args), args);
	printf("[%s] line:%d ret=%d\n", __func__, __LINE__, ret);

	hal_mutex_unlock(arrm->mutex);
	hal_mutex_delete(arrm->mutex);

	free(arrm);

	printf("[%s] line:%d \n", __func__, __LINE__);
	return ret;
}

static int AudioRecordControlRM(tAudioRecordRM *arrm, uint32_t control)
{
	int ret;
	void *args[2] = {0};

	args[0] = (void *)arrm->ar;
	args[1] = (void *)(uintptr_t)control;

	hal_mutex_lock(arrm->mutex);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioRecordControlRM), 1, ARRAY_SIZE(args), args);
	hal_mutex_unlock(arrm->mutex);

	return ret;
}

int AudioRecordStartRM(tAudioRecordRM *arrm)
{
	return AudioRecordControlRM(arrm, 1);
}

int AudioRecordStopRM(tAudioRecordRM *arrm)
{
	return AudioRecordControlRM(arrm, 0);
}

int AudioRecordSetupRM(tAudioRecordRM *arrm, uint32_t rate, uint8_t channels, uint8_t bits)
{
	int ret;
	void *args[4] = {0};

	args[0] = (void *)arrm->ar;
	args[1] = (void *)(uintptr_t)rate;
	args[2] = (void *)(uintptr_t)channels;
	args[3] = (void *)(uintptr_t)bits;

	hal_mutex_lock(arrm->mutex);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioRecordSetupRM), 1, ARRAY_SIZE(args), args);
	hal_mutex_unlock(arrm->mutex);

	return ret;
}

int AudioRecordReadRM(tAudioRecordRM *arrm, void *data, uint32_t size)
{
	int ret;
	void *args[3] = {0};

	args[0] = (void *)arrm->ar;
	args[1] = (void *)data;
	args[2] = (void *)(uintptr_t)size;

	hal_mutex_lock(arrm->mutex);
	hal_dcache_invalidate((unsigned long)data, (unsigned long)size);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioRecordReadRM), 1, ARRAY_SIZE(args), args);
	hal_dcache_invalidate((unsigned long)data, (unsigned long)size);
	hal_mutex_unlock(arrm->mutex);

	return ret;
}
