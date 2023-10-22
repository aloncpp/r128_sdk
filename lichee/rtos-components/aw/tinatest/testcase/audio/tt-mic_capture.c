#include <stdio.h>
#include <stdint.h>
#include <tinatest.h>

extern int cmd_as_test(int argc, char *argv[]);
extern int cmd_arecord(int argc, char ** argv);
static int tt_mic_capture(int argc, char **argv)
{
	int ret;
	int (*func)(int, char **);
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
	char *capture_argv[] = {
		"as_test",
		"-s", "1",
		"-c", "2",
		"-d", "3",
		"-t",
	};
	func = cmd_as_test;
#else
	char *capture_argv[] = {
		"arecord",
		"-D", "hw:audiocodec",
		"-t", /* capture then play */
		"-c", "2",
		"-r", "16000",
		"-f", "16",
		"-d", "5",
	};
	func = cmd_arecord;
#endif

	int capture_argc = sizeof(capture_argv) / sizeof(char *);

	ttips("Start recording (in 5 seconds)\n");

	func(capture_argc, capture_argv);

	ret = ttrue("Finish playing the record. Can you hear the sound?\n");
	if (ret < 0) {
		printf("enter no\n");
		return -1;
	}
	return 0;
}
testcase_init(tt_mic_capture, audiocmic, mic capture for tinatest);
