#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wifimg.h>
#include <wifi_log.h>

wmg_status_t __attribute__((weak)) get_config(char *config_name, char *config_buf)
{
	WMG_DEBUG("unsupport get config function\n");
	return WMG_STATUS_UNSUPPORTED; 
}
