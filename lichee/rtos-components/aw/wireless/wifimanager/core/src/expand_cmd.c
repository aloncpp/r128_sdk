#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wifimg.h>
#include <wifi_log.h>
#include <expand_cmd.h>

wmg_status_t __attribute__((weak)) wmg_linux_send_expand_cmd(char *expand_cmd, void *expand_cb)
{
	WMG_WARNG("wk: unspport linux exp cmd\n");
	return WMG_STATUS_UNSUPPORTED;
}

wmg_status_t __attribute__((weak)) wmg_xrlink_send_expand_cmd(char *expand_cmd, void *expand_cb)
{
	WMG_WARNG("wk: unsupport xrlink exp cmd\n");
	return WMG_STATUS_UNSUPPORTED;
}

wmg_status_t __attribute__((weak)) wmg_freertos_send_expand_cmd(char *expand_cmd, void *expand_cb)
{
	WMG_WARNG("wk: unsupport freertos exp cmd\n");
	return WMG_STATUS_UNSUPPORTED;
}

wmg_status_t wmg_send_expand_cmd(char *expand_cmd, void *expand_cb)
{
	wmg_status_t ret = WMG_STATUS_FAIL;

	if(!strncmp(expand_cmd, "linux", 5)) {
		return wmg_linux_send_expand_cmd((expand_cmd + 7), expand_cb);
	} else if(!strncmp(expand_cmd, "xrlink", 6)) {
		return wmg_xrlink_send_expand_cmd((expand_cmd + 8), expand_cb);
	} else if(!strncmp(expand_cmd, "freertos", 8)) {
		return wmg_freertos_send_expand_cmd((expand_cmd + 10), expand_cb);
	} else {
		WMG_ERROR("unspport os expand_cmd: %s\n", expand_cb);
		return WMG_STATUS_UNSUPPORTED;
	}
}
