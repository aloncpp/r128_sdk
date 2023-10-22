#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <tinatest.h>
#include <hal_timer.h>

extern void cmd_csi_jpeg_offline_test(int argc, char **argv);
extern void cmd_csi_jpeg_online_test(int argc, char **argv);

int tt_camera(int argc, char **argv)
{
	char *yuv_argv[] = {
		"hal_csi_jpeg_offline",
		"-o",
		"nv12",
	};

	char *jpeg_argv[] = {
		"hal_csi_jpeg_offline",
		"-o",
		"jpeg",
	};

	int yuv_argc = sizeof(yuv_argv) / sizeof(yuv_argv[0]);
	int jpeg_argc = sizeof(jpeg_argv) / sizeof(jpeg_argv[0]);

	cmd_csi_jpeg_offline_test(yuv_argc, yuv_argv);
	hal_msleep(200);

	cmd_csi_jpeg_offline_test(jpeg_argc, jpeg_argv);
	hal_msleep(200);

	cmd_csi_jpeg_online_test(argc, argv);

	return 0;
}
testcase_init(tt_camera, camera, camera for tinatest);
