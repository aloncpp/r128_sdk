#ifdef DHCPD_TIMEALT
#include "kernel/os/os_time.h"
#include "dhcp_time.h"

time_t dhcp_time(time_t *timer)
{
	if (!timer)
		return (time_t)XR_OS_GetTime();
	*timer = (time_t)XR_OS_GetTime();
	return *timer;
}

#endif
