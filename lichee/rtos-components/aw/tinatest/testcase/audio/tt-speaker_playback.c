#include <stdio.h>
#include <stdint.h>
#include <tinatest.h>

extern int cmd_as_test(int argc, char *argv[]);
extern int cmd_aplay(int argc, char ** argv);
static int tt_speaker_playback(int argc, char **argv)
{
	int ret;
	int (*func)(int, char **);
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
	char *playback_argv[] = {
		"as_test",
		"-s", "2",
		"-d", "2",
	};
	func = cmd_as_test;
#else
	char *playback_argv[] = {
		"aplay",
		"-D", "default",
		"16K_16bit_1ch"
	};
	func = cmd_aplay;
#endif
	int playback_argc = sizeof(playback_argv) / sizeof(char *);

	ttips("Starting playing with speaker\n");

	func(playback_argc, playback_argv);

	ret = ttrue("Finish playing. Can you hear the sound from speaker?\n");
	if (ret < 0) {
		printf("enter no\n");
		return -1;
	}
	return 0;
}
testcase_init(tt_speaker_playback, audiopspeaker, speaker playback for tinatest);
