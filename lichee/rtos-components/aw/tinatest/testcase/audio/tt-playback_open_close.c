#include <stdio.h>
#include <stdint.h>
#include <tinatest.h>
#include <pthread.h>
#include <unistd.h>
#include <aw-alsa-lib/pcm.h>


static int tt_playback_open_close(int argc, char *argv[])
{
	int loop_count = 100;
	char *card = "default";
	snd_pcm_t *pcm;
	int ret;
	char string[128];

	if (argc == 2) {
		loop_count = atoi(argv[1]);
	}

	if (loop_count < 0) {
		printf("loop_count error :%d\n", loop_count);
		return -1;
	}

	snprintf(string, sizeof(string),
		"playback stream open close test start(count %d).\n",
		loop_count);
	ttips(string);
	while (loop_count--) {
		ret = snd_pcm_open(&pcm, card, SND_PCM_STREAM_PLAYBACK, 0);
		if (ret != 0) {
			printf("snd_pcm_open failed, return %d\n", ret);
			return -1;
		}
		ret = snd_pcm_close(pcm);
		if (ret != 0) {
			printf("snd_pcm_close failed, return %d\n", ret);
			return -1;
		}
		if (loop_count%10 == 0) {
			printf("remain count %d\n", loop_count);
		}
	}

	ttips("playback stream open close test finish.\n");
	return 0;
}
testcase_init(tt_playback_open_close, audiopoc, playback open and close test);
