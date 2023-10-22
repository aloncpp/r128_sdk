#ifndef SUNXI_HAL_CFG_H
#define SUNXI_HAL_CFG_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CONFIG_KERNEL_FREERTOS
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
int32_t hal_cfg_init(void);

#elif defined(CONFIG_OS_MELIS)
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <kapi.h>
int32_t hal_cfg_init(uint8_t *CfgVAddr, uint32_t size);

#else
#error "can not support unknown platform"
#endif

int32_t hal_cfg_exit(void);
int32_t hal_cfg_get_sub_keyvalue(char *MainKeyName, char *SubKeyName, void *value, int32_t type);
int32_t hal_cfg_get_keyvalue(char *SecName, char *KeyName, int32_t Value[], int32_t Count);
int32_t hal_cfg_get_sec_keycount(char *SecName);
int32_t hal_cfg_get_sec_count(void);
int32_t hal_cfg_get_gpiosec_keycount(char *GPIOSecName);
int32_t hal_cfg_get_gpiosec_data(char *GPIOSecName, void *pGPIOCfg, int32_t GPIONum);

#ifdef __cplusplus
}
#endif

#endif
