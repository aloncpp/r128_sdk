#ifndef __BOOT_REASON_H__
#define __BOOT_REASON_H__

extern int app_write_boot_reason_when_reboot(void);
extern int app_write_boot_reason_when_panic(void);
extern int app_clear_boot_reason(void);

extern uint32_t app_get_current_boot_reason(void);
extern const char* boot_reason_to_desc_str(uint32_t boot_reason);

#endif /* __BOOT_REASON_H__ */
