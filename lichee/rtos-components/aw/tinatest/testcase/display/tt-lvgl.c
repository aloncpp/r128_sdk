#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <tinatest.h>

extern int lvgl_main(int argc, char *argv[]);

int tt_lvgl(int argc, char **argv)
{
	int ret = 0;

	char *lvgl_argv[] = {
		"lv_examples",
		"0",
	};

	int lvgl_argc = sizeof(lvgl_argv) / sizeof(lvgl_argv[0]);

	ret = lvgl_main(lvgl_argc, lvgl_argv);

	return ret;
}
testcase_init(tt_lvgl, lvgl, lvgl for tinatest);
