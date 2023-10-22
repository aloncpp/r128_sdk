#ifndef __COMMON_H
#define __COMMON_H

#include <portmacro.h>
#include <FreeRTOS-Plus-CLI/FreeRTOS_CLI.h>
#ifdef CONFIG_COMPONENTS_FREERTOS_CLI
#include <console.h>
#endif
#include <aw_common.h>
#include <aw_list.h>

#if __cplusplus
extern "C" {
#endif
typedef struct {
	snd_pcm_t *handle;
	snd_pcm_format_t format;
	unsigned int rate;
	unsigned int channels;
	snd_pcm_uframes_t period_size;
	snd_pcm_uframes_t buffer_size;

	snd_pcm_uframes_t frame_bytes;
	snd_pcm_uframes_t chunk_size;

	unsigned in_aborting;
	unsigned int capture_duration;
} audio_mgr_t;

void xrun(snd_pcm_t *handle);
void do_other_test(snd_pcm_t *handle);
void do_pause(snd_pcm_t *handle);

int pcm_read(snd_pcm_t *handle, const char *data, snd_pcm_uframes_t frames_total,
		unsigned int frame_bytes);
int arecord_data(const char *card_name, snd_pcm_format_t format, unsigned int rate,
		unsigned int channels, const void *data, unsigned int datalen);

int pcm_write(snd_pcm_t *handle, char *data, snd_pcm_uframes_t frames_total,
		unsigned int frame_bytes);
int aplay_data(const char *card_name, snd_pcm_format_t format, unsigned int rate,
		unsigned int channels, const char *data, unsigned int datalen,
		unsigned int loop_count);

int set_param(snd_pcm_t *handle, snd_pcm_format_t format,
		unsigned int rate, unsigned int channels,
		snd_pcm_uframes_t period_size, snd_pcm_uframes_t buffer_size);

audio_mgr_t *audio_mgr_create(void);
void audio_mgr_dump_args(audio_mgr_t *audio_mgr);
void audio_mgr_release(audio_mgr_t *mgr);

int amixer_sset_enum_ctl(const char *card_name, const char *ctl_name,
			const char *ctl_val);

void FUNCTION_THREAD_STOP_LINE_PRINTF(const char *string);
void FUNCTION_THREAD_LINE_PRINTF(const char *string, const unsigned int line);
void FUNCTION_THREAD_START_LINE_PRINTF(const char *string);

#if __cplusplus
};  // extern "C"
#endif
#endif /* __COMMON_H */
