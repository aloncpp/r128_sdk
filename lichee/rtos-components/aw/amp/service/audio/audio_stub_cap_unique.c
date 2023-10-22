#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

#include "sunxi_amp.h"
#include <hal_cache.h>
#include <hal_mutex.h>
#include <console.h>

#include <AudioSystem.h>


typedef struct {
	tAudioRecord *ar;
	uint32_t buffer_bytes;
	/*uint32_t buffer_threshold;*/
	uint32_t buffer_pos;
	hal_mutex_t mutex;
	void *buffer_addr;
	uint8_t state;
} tAudioRecordRM;

/* cache align */
#define AR_RM_BUFFER_BYTES_DEFAULT	(2*2*960*2)
tAudioRecord *AudioRecordCreate(const char *name)
{
	tAudioRecord *ar;
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
	args[1] = (void *)len;

	hal_dcache_clean((unsigned long)align_name, (unsigned long)len);
	ar = (tAudioRecord *)func_stub(RPCCALL_AUDIO(AudioRecordCreateRM), 1, ARRAY_SIZE(args), args);
	amp_align_free(align_name);

	if (!ar)
		return NULL;
	arrm = malloc(sizeof(tAudioRecordRM));
	if (!arrm)
		goto err;
	arrm->ar = ar;

	arrm->buffer_bytes = AR_RM_BUFFER_BYTES_DEFAULT;
	arrm->buffer_pos = 0;
	/*arrm->buffer_threshold = arrm->buffer_bytes;*/
	arrm->buffer_addr = amp_align_malloc(arrm->buffer_bytes);
	if(!arrm->buffer_addr)
		goto err;

	arrm->mutex = hal_mutex_create();

	arrm->state = AT_RM_STATE_SETUP;

	return (tAudioRecord *)arrm;
err:
	if (arrm->buffer_addr)
		free(arrm->buffer_addr);
	if (arrm)
		free(arrm);
	return NULL;
}

int AudioRecordDestroy(tAudioRecord *ar)
{
	int ret;
	tAudioRecordRM *arrm = (tAudioRecordRM *)ar;
	void *args[1] = {0};

	printf("[%s] line:%d \n", __func__, __LINE__);
	hal_mutex_lock(arrm->mutex);
	args[0] = (void *)arrm->ar;
	ret = (int)func_stub(RPCCALL_AUDIO(AudioRecordDestroyRM), 1, ARRAY_SIZE(args), args);
	printf("[%s] line:%d ret=%d\n", __func__, __LINE__, ret);

	if (arrm->buffer_addr)
		free(arrm->buffer_addr);

	hal_mutex_unlock(arrm->mutex);
	hal_mutex_delete(arrm->mutex);

	free(arrm);

	printf("[%s] line:%d \n", __func__, __LINE__);
	return ret;
}

static int AudioRecordControlRM(tAudioRecord *ar, uint32_t control)
{
	int ret;
	tAudioRecordRM *arrm = (tAudioRecordRM *)ar;
	void *args[2] = {0};

	args[0] = (void *)arrm->ar;
	args[1] = (void *)control;

	hal_mutex_lock(arrm->mutex);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioRecordControlRM), 1, ARRAY_SIZE(args), args);
	hal_mutex_unlock(arrm->mutex);

	return ret;
}

int AudioRecordStart(tAudioRecord *ar)
{
	return AudioRecordControlRM(ar, 1);
}

int AudioRecordStop(tAudioRecord *ar)
{
	return AudioRecordControlRM(ar, 0);
}

int AudioRecordSetup(tAudioRecord *ar, uint32_t rate, uint8_t channels, uint8_t bits)
{
	int ret;
	tAudioRecordRM *arrm = (tAudioRecordRM *)ar;
	void *args[4] = {0};

	args[0] = (void *)arrm->ar;
	args[1] = (void *)rate;
	args[2] = (void *)(uint32_t)channels;
	args[3] = (void *)(uint32_t)bits;

	hal_mutex_lock(arrm->mutex);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioRecordSetupRM), 1, ARRAY_SIZE(args), args);
	hal_mutex_unlock(arrm->mutex);

	return ret;
}

/* keep mutex */
static int _AudioRecordReadRM(tAudioRecordRM *arrm, void *data, uint32_t size)
{
	int ret;
	void *args[3] = {0};

	args[0] = (void *)arrm->ar;
	args[1] = (void *)data;
	args[2] = (void *)size;

	hal_dcache_invalidate((unsigned long)data, (unsigned long)size);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioRecordReadRM), 1, ARRAY_SIZE(args), args);
	hal_dcache_invalidate((unsigned long)data, (unsigned long)size);

	return ret;
}

int AudioRecordRead(tAudioRecord *ar, void *data, uint32_t size)
{
	tAudioRecordRM *arrm = (tAudioRecordRM *)ar;
	uint32_t avail;
	void *addr;
	uint32_t written = 0, ofs = 0;
	int ret = 0;

	hal_mutex_lock(arrm->mutex);

	while (size > 0) {
		if (!arrm->buffer_pos) {
			ret = _AudioRecordReadRM(arrm, arrm->buffer_addr, arrm->buffer_bytes);
			if (ret < 0) {
				printf("ar rm read failed, ret=%d\n", ret);
				hal_mutex_unlock(arrm->mutex);
				return ret;
			}
			if (ret != arrm->buffer_bytes) {
				printf("[%s] line:%d ret=%d but expect %d\n", __func__, __LINE__,
						ret, arrm->buffer_bytes);
			}
			arrm->buffer_pos = arrm->buffer_bytes;
		}
		if (arrm->buffer_pos == arrm->buffer_bytes)
			avail = arrm->buffer_bytes;
		else
			avail = arrm->buffer_bytes - arrm->buffer_pos;
		if (avail < size)
			written = avail;
		else
			written = size;
		/*printf("[%s] line:%d ar=%p size=%u, avail=%u\n", __func__, __LINE__,*/
				/*arrm->ar, size, avail);*/
		addr = arrm->buffer_addr + arrm->buffer_pos;
		/* fill data into buffer_addr */
		memcpy(data + ofs, addr, written);
		size -= written;
		arrm->buffer_pos += written;
		ofs += written;

		if (arrm->buffer_pos != arrm->buffer_bytes) {
			#if 1
			if (size != 0)
				printf("size=%u, written=%u, maybe error\n",
					size, written);
			#endif
			continue;
		}
		arrm->buffer_pos = 0;
	}

	hal_mutex_unlock(arrm->mutex);

	return ret;
}
