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
	tAudioTrack *at;
	uint32_t buffer_bytes;
	/*uint32_t buffer_threshold;*/
	uint32_t buffer_pos;
	hal_mutex_t mutex;
	void *buffer_addr;
	uint8_t state;
} tAudioTrackRM;

/* cache align */
#define AT_RM_BUFFER_BYTES_DEFAULT	(2*2*960*2)

tAudioTrack *AudioTrackCreate(const char *name)
{
	tAudioTrack *at;
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
	args[1] = (void *)len;
	hal_dcache_clean((unsigned long)align_name, (unsigned long)len);
	at = (tAudioTrack *)func_stub(RPCCALL_AUDIO(AudioTrackCreateRM), 1, ARRAY_SIZE(args), args);
	amp_align_free(align_name);

	if (!at)
		return NULL;
	atrm = malloc(sizeof(tAudioTrackRM));
	if (!atrm)
		goto err;
	atrm->at = at;

	atrm->buffer_bytes = AT_RM_BUFFER_BYTES_DEFAULT;
	atrm->buffer_pos = 0;
	/*atrm->buffer_threshold = atrm->buffer_bytes;*/
	atrm->buffer_addr = amp_align_malloc(atrm->buffer_bytes);
	if(!atrm->buffer_addr)
		goto err;

	atrm->mutex = hal_mutex_create();

	atrm->state = AT_RM_STATE_SETUP;

	return (tAudioTrack *)atrm;
err:
	if (atrm->buffer_addr)
		free(atrm->buffer_addr);
	if (atrm)
		free(atrm);
	return NULL;
}

int AudioTrackDestroy(tAudioTrack *at)
{
	int ret;
	tAudioTrackRM *atrm = (tAudioTrackRM *)at;
	void *args[1] = {0};

	hal_mutex_lock(atrm->mutex);
	args[0] = (void *)atrm->at;
	ret = (int)func_stub(RPCCALL_AUDIO(AudioTrackDestroyRM), 1, ARRAY_SIZE(args), args);

	if (atrm->buffer_addr)
		free(atrm->buffer_addr);

	hal_mutex_unlock(atrm->mutex);
	hal_mutex_delete(atrm->mutex);

	free(atrm);

	return ret;
}

static int AudioTrackControlRM(tAudioTrack *at, uint32_t control)
{
	int ret;
	tAudioTrackRM *atrm = (tAudioTrackRM *)at;
	void *args[2] = {0};

	args[0] = (void *)atrm->at;
	args[1] = (void *)control;

	hal_mutex_lock(atrm->mutex);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioTrackControlRM), 1, ARRAY_SIZE(args), args);
	hal_mutex_unlock(atrm->mutex);

	return ret;
}

int AudioTrackStart(tAudioTrack *at)
{
	return AudioTrackControlRM(at, 1);
}

int AudioTrackStop(tAudioTrack *at)
{
	return AudioTrackControlRM(at, 0);
}

int AudioTrackSetup(tAudioTrack *at, uint32_t rate, uint8_t channels, uint8_t bits)
{
	int ret;
	tAudioTrackRM *atrm = (tAudioTrackRM *)at;
	void *args[4] = {0};

	args[0] = (void *)atrm->at;
	args[1] = (void *)rate;
	args[2] = (void *)(uint32_t)channels;
	args[3] = (void *)(uint32_t)bits;

	hal_mutex_lock(atrm->mutex);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioTrackSetupRM), 1, ARRAY_SIZE(args), args);
	hal_mutex_unlock(atrm->mutex);

	return ret;
}

/* keep mutex */
static int _AudioTrackWriteRM(tAudioTrackRM *atrm, void *data, uint32_t size)
{
	int ret;
	void *args[3] = {0};

	args[0] = (void *)atrm->at;
	args[1] = (void *)data;
	args[2] = (void *)size;

	hal_dcache_clean((unsigned long)data, (unsigned long)size);
	ret = (int)func_stub(RPCCALL_AUDIO(AudioTrackWriteRM), 1, ARRAY_SIZE(args), args);

	return ret;
}

#if 0
int AudioTrackWrite(tAudioTrack *at, void *data, uint32_t size)
{
	tAudioTrackRM *atrm = (tAudioTrackRM *)at;
	uint32_t avail;
	void *addr;
	uint32_t written = 0, ofs = 0;
	int ret = 0, xfer = 0;;

	printf("[%s] line:%d size=%d\n", __func__, __LINE__, size);
	hal_mutex_lock(atrm->mutex);

	while (size > 0) {
		avail = atrm->buffer_bytes - atrm->buffer_pos;
		if (avail < size)
			written = avail;
		else
			written = size;
		printf("[%s] line:%d size=%d, avail=%d, written=%d\n", __func__, __LINE__, size, avail, written);
		/*printf("[%s] line:%d at=%p size=%u, avail=%u\n", __func__, __LINE__,*/
				/*atrm->at, size, avail);*/

		addr = atrm->buffer_addr + atrm->buffer_pos;
		printf("[%s] line:%d buffer_addr=%p, addr=%p \n", __func__, __LINE__, atrm->buffer_addr, addr);
		/* fill data into buffer_addr */
		memcpy(addr, data + ofs, written);
		size -= written;
		atrm->buffer_pos += written;
		ofs += written;
		xfer += written;

		if (atrm->buffer_pos != atrm->buffer_bytes) {
			#if 1
			if (size != 0)
				printf("size=%u, written=%u, maybe error\n",
					size, written);
			#endif
			continue;
		}
		atrm->buffer_pos = 0;
		ret = _AudioTrackWriteRM(atrm, atrm->buffer_addr, atrm->buffer_bytes);
		printf("[%s] line:%d ret=%d buffer_bytes=%d\n", __func__, __LINE__, ret, atrm->buffer_bytes);
		if (ret < 0) {
			printf("at rm write failed, ret=%d\n", ret);
			hal_mutex_unlock(atrm->mutex);
			return ret;
		}
		if (ret != atrm->buffer_bytes) {
			printf("[%s] line:%d ret=%d but expect %d\n", __func__, __LINE__,
					ret, atrm->buffer_bytes);
		}
	}
	printf("[%s] line:%d ret=%d\n", __func__, __LINE__, ret);

	hal_mutex_unlock(atrm->mutex);

	return ret != 0 ? ret : xfer;
}
#else
int AudioTrackWrite(tAudioTrack *at, void *data, uint32_t size)
{
	tAudioTrackRM *atrm = (tAudioTrackRM *)at;
	uint32_t avail;
	void *addr;
	uint32_t written = 0, xfer = 0;
	int ret = 0;

	hal_mutex_lock(atrm->mutex);

	while (size > 0) {
		avail = atrm->buffer_bytes;
		if (avail < size)
			written = avail;
		else
			written = size;

		addr = atrm->buffer_addr;
		data = data + xfer;
		/*printf("[%s] line:%d addr=%p data=%p\n", __func__, __LINE__, addr, data);*/
		memcpy(addr, data, written);
		size -= written;
		xfer += written;

		ret = _AudioTrackWriteRM(atrm, atrm->buffer_addr, written);
		/*printf("[%s] line:%d ret=%d buffer_bytes=%d\n", __func__, __LINE__, ret, atrm->buffer_bytes);*/
		if (ret < 0) {
			printf("at rm write failed, ret=%d\n", ret);
			break;
		}
	}

	hal_mutex_unlock(atrm->mutex);

	return xfer != 0 ? xfer : ret;
}
#endif
