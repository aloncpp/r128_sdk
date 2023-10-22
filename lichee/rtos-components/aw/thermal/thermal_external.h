
#ifndef __THERMAL_EXTERNAL_H__
#define __THERMAL_EXTERNAL_H__


int thermal_external_get_zone_number(void);
int thermal_external_get_temp(int *id_buf, int *temp_buf, int num);

#endif
