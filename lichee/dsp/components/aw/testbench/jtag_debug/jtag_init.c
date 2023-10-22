#include <stdio.h>
#include <platform.h>
#include <aw_io.h>
#include <console.h>

int cmd_jtag_init(int argc, char *argv)
{
	long unsigned int reg_val;

#if defined(CONFIG_ARCH_SUN50IW11)

	reg_val = readl(SUNXI_R_GPIO_PBASE + 0x0024);
	reg_val &= ~((0x7 << 0) | (0x7 << 4) | (0x7 << 8) | (0x7 << 20));
	reg_val |= (0x3 << 0) | (0x3 << 4) | (0x3 << 8) | (0x3 << 20);
	writel(reg_val, SUNXI_R_GPIO_PBASE + 0x0024);

#endif

#if defined(CONFIG_ARCH_SUN8IW20)
	//PE14-17 select jtag
	reg_val = readl(SUNXI_GPIO_BASE + 0x00c4);
	reg_val &= ~((0xf << 24) | (0xf << 28));
	reg_val |= (0x3 << 24) | (0x3 << 28);
	writel(reg_val, SUNXI_GPIO_BASE + 0x00c4);

	reg_val = readl(SUNXI_GPIO_BASE + 0x00c8);
	reg_val &= ~((0xf << 0) | (0xf << 4));
	reg_val |= (0x3 << 0) | (0x3 << 4);
	writel(reg_val, SUNXI_GPIO_BASE + 0x00c8);

#endif

	printf("JTAG Init success!\n");

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_jtag_init, jtag_init, Init S_JTAG for DSP debugging);
