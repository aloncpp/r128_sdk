#include <stdio.h>
#include <stdint.h>
#include <sunxi_hal_rtc.h>
#include <hal_cmd.h>

#define RTC_BOOT_REASON_REG_INDEX 7

typedef enum {
	SUNXI_BOOT_REASON_COLD_BOOT = 1,
	/* These enums below belong to warm boot(hot boot) */
	SUNXI_BOOT_REASON_CHIP_RESET_PIN,
	SUNXI_BOOT_REASON_ARM_NORMAL_REBOOT,
	SUNXI_BOOT_REASON_ARM_PANIC_REBOOT,
	SUNXI_BOOT_REASON_ARM_WATCHDOG_RESET,
	SUNXI_BOOT_REASON_RV_NORMAL_REBOOT,
	SUNXI_BOOT_REASON_RV_PANIC_REBOOT,
	SUNXI_BOOT_REASON_RV_WATCHDOG_RESET,
	SUNXI_BOOT_REASON_DSP_NORMAL_REBOOT,
	SUNXI_BOOT_REASON_DSP_PANIC_REBOOT,
	SUNXI_BOOT_REASON_DSP_WATCHDOG_RESET,
} sunxi_boot_reason_t;

#define APP_INFO_FIELD_MASK 0x7FFF0000
#define APP_INFO_FIELD_SHIFT 16

#define BOOT_INFO_INVALID_FIELD_MASK 0x8000
#define BOOT_INFO_FIELD_MASK 0x00007FFF
#define BOOT_INFO_FIELD_SHIFT 0

static inline uint32_t get_app_info_field(uint32_t boot_reason_reg)
{
	return ((boot_reason_reg & APP_INFO_FIELD_MASK) >> APP_INFO_FIELD_SHIFT);
}

static inline void clear_app_info_field(uint32_t *boot_reason_reg)
{
	*boot_reason_reg &= ~(APP_INFO_FIELD_MASK);
}

static inline void set_app_info_field(uint32_t *boot_reason_reg, uint32_t app_info)
{
	*boot_reason_reg &= ~(APP_INFO_FIELD_MASK);
	*boot_reason_reg |= ((app_info << APP_INFO_FIELD_SHIFT) & APP_INFO_FIELD_MASK);
}

static inline uint32_t get_boot_info_field(uint32_t boot_reason_reg)
{
	return ((boot_reason_reg & (BOOT_INFO_INVALID_FIELD_MASK | BOOT_INFO_FIELD_MASK))
			>> BOOT_INFO_FIELD_SHIFT);
}

static inline void set_boot_info_field(uint32_t *boot_reason_reg, uint32_t boot_info)
{
	*boot_reason_reg &= ~(BOOT_INFO_INVALID_FIELD_MASK | BOOT_INFO_FIELD_MASK);
	*boot_reason_reg |= (boot_info & (BOOT_INFO_INVALID_FIELD_MASK | BOOT_INFO_FIELD_MASK));
}

static inline void mark_boot_info_invalid(uint32_t *boot_info)
{
	*boot_info |= BOOT_INFO_INVALID_FIELD_MASK;
}

static inline int is_boot_info_valid(uint32_t boot_info)
{
	if (boot_info & BOOT_INFO_INVALID_FIELD_MASK) {
		return 0;
	}
	return 1;
}

static int is_app_info_valid(uint32_t app_info)
{
	switch (app_info) {
	case SUNXI_BOOT_REASON_ARM_NORMAL_REBOOT:
	case SUNXI_BOOT_REASON_ARM_PANIC_REBOOT:
	case SUNXI_BOOT_REASON_RV_NORMAL_REBOOT:
	case SUNXI_BOOT_REASON_RV_PANIC_REBOOT:
	case SUNXI_BOOT_REASON_DSP_NORMAL_REBOOT:
	case SUNXI_BOOT_REASON_DSP_PANIC_REBOOT:
		return 1;
	default:
		return 0;
	}

	return 0;
}

static int is_painc_reboot(uint32_t app_info)
{
	switch (app_info) {
	case SUNXI_BOOT_REASON_ARM_PANIC_REBOOT:
	case SUNXI_BOOT_REASON_RV_PANIC_REBOOT:
	case SUNXI_BOOT_REASON_DSP_PANIC_REBOOT:
		return 1;
	default:
		return 0;
	}

	return 0;
}

static int write_boot_reason_reg(uint32_t data)
{
    do
    {
        hal_rtc_write_data(RTC_BOOT_REASON_REG_INDEX, data);
        isb();
    } while (hal_rtc_read_data(RTC_BOOT_REASON_REG_INDEX) != data);

    return 0;
}

static int read_boot_reason_reg(uint32_t* reg_data)
{
    *reg_data = hal_rtc_read_data(RTC_BOOT_REASON_REG_INDEX);
    return 0;
}

static int app_write_boot_reason(uint32_t boot_reason)
{
	uint32_t boot_reason_reg = 0;

	if (!is_app_info_valid(boot_reason))
		return -1;

	read_boot_reason_reg(&boot_reason_reg);
	set_app_info_field(&boot_reason_reg, boot_reason);
	return write_boot_reason_reg(boot_reason_reg);
}

int app_write_boot_reason_when_reboot(void)
{
	int ret = 0;
#if defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_ARCH_ARM_CORTEX_M33) /* CPU */
	ret = app_write_boot_reason(SUNXI_BOOT_REASON_ARM_NORMAL_REBOOT);
#elif defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_ARCH_RISCV_C906) /* RISCV */
	ret = app_write_boot_reason(SUNXI_BOOT_REASON_RV_NORMAL_REBOOT);
#elif defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_ARCH_DSP) /* DSP */
	ret = app_write_boot_reason(SUNXI_BOOT_REASON_DSP_NORMAL_REBOOT);
#endif
	return ret;
}

int app_write_boot_reason_when_panic(void)
{
	int ret = 0;
#if defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_ARCH_ARM_CORTEX_M33) /* CPU */
	ret = app_write_boot_reason(SUNXI_BOOT_REASON_ARM_PANIC_REBOOT);
#elif defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_ARCH_RISCV_C906) /* RISCV */
	ret = app_write_boot_reason(SUNXI_BOOT_REASON_RV_PANIC_REBOOT);
#elif defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_ARCH_DSP) /* DSP */
	ret = app_write_boot_reason(SUNXI_BOOT_REASON_DSP_PANIC_REBOOT);
#endif
	return ret;
}

int app_clear_boot_reason(void)
{
	uint32_t boot_reason_reg = 0;

	read_boot_reason_reg(&boot_reason_reg);
	clear_app_info_field(&boot_reason_reg);
	return write_boot_reason_reg(boot_reason_reg);
}

uint32_t app_get_current_boot_reason(void)
{
	uint32_t boot_reason_reg = 0;
	read_boot_reason_reg(&boot_reason_reg);
	return get_boot_info_field(boot_reason_reg);
}

#define ENUM_VALUE_TO_STR(val) #val
const char* boot_reason_to_desc_str(uint32_t boot_reason)
{
	const char *str = NULL;

	if (!is_boot_info_valid(boot_reason)) {
		return "invalid";
	}

	switch (boot_reason) {
	case SUNXI_BOOT_REASON_COLD_BOOT:
		str = ENUM_VALUE_TO_STR(SUNXI_BOOT_REASON_COLD_BOOT);
		break;
	case SUNXI_BOOT_REASON_CHIP_RESET_PIN:
		str = ENUM_VALUE_TO_STR(SUNXI_BOOT_REASON_CHIP_RESET_PIN);
		break;
	case SUNXI_BOOT_REASON_ARM_NORMAL_REBOOT:
		str = ENUM_VALUE_TO_STR(SUNXI_BOOT_REASON_ARM_NORMAL_REBOOT);
		break;
	case SUNXI_BOOT_REASON_ARM_PANIC_REBOOT:
		str = ENUM_VALUE_TO_STR(SUNXI_BOOT_REASON_ARM_PANIC_REBOOT);
		break;
	case SUNXI_BOOT_REASON_ARM_WATCHDOG_RESET:
		str = ENUM_VALUE_TO_STR(SUNXI_BOOT_REASON_ARM_WATCHDOG_RESET);
		break;
	case SUNXI_BOOT_REASON_RV_NORMAL_REBOOT:
		str = ENUM_VALUE_TO_STR(SUNXI_BOOT_REASON_RV_NORMAL_REBOOT);
		break;
	case SUNXI_BOOT_REASON_RV_PANIC_REBOOT:
		str = ENUM_VALUE_TO_STR(SUNXI_BOOT_REASON_RV_PANIC_REBOOT);
		break;
	case SUNXI_BOOT_REASON_RV_WATCHDOG_RESET:
		str = ENUM_VALUE_TO_STR(SUNXI_BOOT_REASON_RV_WATCHDOG_RESET);
		break;
	case SUNXI_BOOT_REASON_DSP_NORMAL_REBOOT:
		str = ENUM_VALUE_TO_STR(SUNXI_BOOT_REASON_DSP_NORMAL_REBOOT);
		break;
	case SUNXI_BOOT_REASON_DSP_PANIC_REBOOT:
		str = ENUM_VALUE_TO_STR(SUNXI_BOOT_REASON_DSP_PANIC_REBOOT);
		break;
	case SUNXI_BOOT_REASON_DSP_WATCHDOG_RESET:
		str = ENUM_VALUE_TO_STR(SUNXI_BOOT_REASON_DSP_WATCHDOG_RESET);
		break;
	default:
		str = "unknown";
		break;
	}
	return str;
}

static int cmd_boot_reason(int argc, char **argv)
{
	uint32_t boot_reason_reg = 0, boot_info = 0;
	read_boot_reason_reg(&boot_reason_reg);
	boot_info = get_boot_info_field(boot_reason_reg);
	printf("current boot reason: %s(0x%08x)\n", boot_reason_to_desc_str(boot_info), boot_info);
}

FINSH_FUNCTION_EXPORT_CMD(cmd_boot_reason, boot_reason, show boot reason)
