#ifndef CAMERA_CMD_H
#define CAMERA_CMD_H

#include "format_convert.h"
#include "display.h"
#include "show_word.h"

#if defined(CONFIG_CSI_CAMERA)
int csi_preview_test(int argc, const char **argv);
#endif

#if defined(CONFIG_USB_CAMERA)
int uvc_preview_test(int argc, const char **argv);
#endif

#endif  /*CAMERA_CMD_H*/
