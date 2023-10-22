#include <stdio.h>
#include <platform.h>
#include <aw_io.h>
#include <console.h>

#define DSP1_POWER_USAGE \
	"dsp1_power usage:\n" \
	"param 1: 0-off; 1-on\n"

static void dsp1_power_usage()
{
	printf("%s", DSP1_POWER_USAGE);
}

int cmd_dsp1_power(int argc, char *argv[])
{
	long unsigned int reg_val;
	unsigned int onoff;
	int ret;

	if (argc != 2) {
		dsp1_power_usage();
		return -1;
	}

	ret = sscanf(argv[1], "%d", &onoff);

	switch (onoff) {
	case 0:
		reg_val = readl(SUNXI_R_PRCM_PBASE + 0x0258);
		reg_val |= (1 << 16);
		writel(reg_val, SUNXI_R_PRCM_PBASE + 0x0258);

		reg_val = readl(SUNXI_R_PRCM_PBASE + 0x0258);
		reg_val |= (0xff << 8);
		writel(reg_val, SUNXI_R_PRCM_PBASE + 0x0258);

		printf("DSP1 power down success!\n");
		break;
	case 1:
		reg_val = readl(SUNXI_R_PRCM_PBASE + 0x0258);
		reg_val &= ~(0xff << 8);
		writel(reg_val, SUNXI_R_PRCM_PBASE + 0x0258);

		reg_val = readl(SUNXI_R_PRCM_PBASE + 0x0258);
		reg_val &= ~(1 << 16);
		writel(reg_val, SUNXI_R_PRCM_PBASE + 0x0258);

		printf("DSP1 power on success!\n");
		break;
	}

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_dsp1_power, dsp1_power, DSP1 power control);
