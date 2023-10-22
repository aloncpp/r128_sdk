#ifndef _BSP_H_
#define _BSP_H

#ifdef  __cplusplus
extern "C"
{
#endif

void bt_bsp_init(void);
void bt_reset_gpio(void);

void bt_reset_set(void);
void bt_reset_clear(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
