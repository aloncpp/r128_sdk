#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hal_cmd.h>
#include "camera_cmd.h"

static void camera_preview_show_help(void)
{
	printf("\nUsage:\n"\
		"\tcamera_preview csi\n"\
		"\tcamera_preview uvc\n"\
		"\n- - - - - - - - - - - - - - - - - - - - -\n"\
		"Meaning:\n"\
		"\tcsi  : Camera use CSI driver\n"\
		"\tuvc  : Camera use UVC driver\n");
}

static int cmd_csi_preview_test(int argc, const char **argv)
{
#if defined(CONFIG_CSI_CAMERA)
	return csi_preview_test(argc, argv);
#else
	printf("[%s]: ERR: Can't find command config!\n", __func__);
	return -1;
#endif
}

static int cmd_uvc_preview_test(int argc, const char **argv)
{
#if defined(CONFIG_USB_CAMERA)
	return uvc_preview_test(argc, argv);
#else
	printf("[%s]: ERR: Can't find command config!\n", __func__);
	return -1;
#endif
}

static int cmd_camera_preview_test(int argc, const char **argv)
{
	int ret = -1;
	if (argc < 2) {
		printf("[%s]: ERR: command error\n", __func__);
		camera_preview_show_help();
		return -1;
	}

	if (!strcmp(argv[1], "csi"))
		ret = cmd_csi_preview_test(argc, argv);
	else if (!strcmp(argv[1], "uvc"))
		ret = cmd_uvc_preview_test(argc, argv);

	if (ret == 0)
		return 0;

	camera_preview_show_help();
	return -1;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_camera_preview_test, camera_preview, camera preview test)
