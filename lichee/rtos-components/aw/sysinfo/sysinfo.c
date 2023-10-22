#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <hal_efuse.h>
#include <hal_cmd.h>

int cmd_sysinfo(int argc, char **argv)
{
	int secure_flag = 0;
	int rotpk_flag = 0;
	int i = 0;
	unsigned char chipid[16] = {0};
	char chipid_str[129] = {0};

	secure_flag = hal_efuse_get_security_mode();
	rotpk_flag = hal_efuse_get_rotpk_flag();

	if (hal_efuse_read("chipid", chipid, 128) != 16) {
		printf("read chipid error\n!");
		return -1;
	}
	memset(chipid_str, 0, 129);
	for (i = 0; i < 4; i++)
		sprintf(chipid_str + i*8, "%p", (unsigned int *)chipid[i*4]);

#ifdef CONFIG_ARCH_SUN20IW2
	printf("platform    : aw1883\n");
#endif
	printf("secure      : %d\n", secure_flag);
	printf("rotpk       : %d\n", rotpk_flag);
	printf("chipid      : %s\n", chipid_str);

}
FINSH_FUNCTION_EXPORT_CMD(cmd_sysinfo, sysinfo, show sysinfo);
