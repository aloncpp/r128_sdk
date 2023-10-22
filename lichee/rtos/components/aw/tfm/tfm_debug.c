#include <stdio.h>
#include "console.h"
#include <FreeRTOS.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <task.h>
#include <getopt.h>
#include <io.h>
#include <awlog.h>
#ifdef CONFIG_DRIVERS_EFUSE
#include <sunxi_hal_efuse.h>
#endif
#include "tfm_sunxi_debug_nsc.h"

int cmd_tfm_hexdump(int argc, char ** argv)
{
	uint32_t addr;
	uint32_t len;
	char *err = NULL;

	if(argc != 3) {
		printf("Argument Error!\n");
		printf("Usage: tfm_hexdump <addr> <len>\n");
		return -1;
	}

	addr = strtoul(argv[1], &err, 0);
	len = strtoul(argv[2], &err, 0);

	tfm_sunxi_hexdump(addr, len);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_tfm_hexdump, tfm_hexdump, tfm hexdump);

int cmd_show_sau(int argc, char ** argv)
{
	tfm_sunxi_sau_info_printf();

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_show_sau, sau, show sau info);

int cmd_tfm_mem_readl(int argc, char **argv)
{
	char *err = NULL;
	uint32_t start_addr, end_addr ;
	uint32_t len;

	if (NULL == argv[1]) {
		printf("Argument Error!\n");
		printf("Usage: tfm_mem_readl <addr> [len]\n");
		return -1;
	}

	if (argv[2]) {
		start_addr = strtoul(argv[1], &err, 0);

		len = strtoul(argv[2], &err, 0);
		end_addr = start_addr + len;

		printf("start_addr=0x%08x end_addr=0x%08x\n", start_addr, end_addr);
		for (; start_addr <= end_addr;) {
		    printf("reg_addr[0x%08x]=0x%08x \n", start_addr,
						tfm_sunxi_readl(start_addr));
		    start_addr += 4;
		}
	} else {
		start_addr = strtoul(argv[1], &err, 0);
		printf("reg_addr[0x%08x]=0x%08x \n", start_addr,
						tfm_sunxi_readl(start_addr));
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_tfm_mem_readl, tfm_mem_readl, tfm mem read word);

int cmd_tfm_mem_writel(int argc, char ** argv)
{
	uint32_t reg_addr, reg_value ;
	char *err = NULL;

	if(argc < 3) {
		printf("Argument Error!\n");
		printf("Usage: tfm_mem_writel <addr> <value>\n");
		return -1;
	}

	if ((NULL == argv[1]) || (NULL == argv[2])) {
		printf("Argument Error!\n");
		return -1;
	}

	reg_addr = strtoul(argv[1], &err, 0);
	reg_value = strtoul(argv[2], &err, 0);

	tfm_sunxi_writel(reg_addr, reg_value);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_tfm_mem_writel, tfm_mem_writel, tfm mem write word);

#ifdef CONFIG_DRIVERS_EFUSE
int cmd_tfm_efuse_read(int argc, char ** argv)
{
	unsigned char *key_data = NULL;
	efuse_key_map_new_t *key_map = NULL;
	int read_size = 0;
	int ret = 0;

	if (argc != 2) {
		printf("Argument Error!\n");
		printf("Usage: tfm_efuse_read <key_name>\n");
		return -1;
	}

	key_map = efuse_search_key_by_name(argv[1]);
	if (key_map->size == 0) {
		printf("unknow key name: %s\n", argv[1]);
		return -1;
	}

	key_data = malloc(key_map->size >> 3);
	if (!key_data) {
		printf("malloc %d bytes error!\n", key_map->size >> 3);
		return -1;
	}

	read_size = tfm_sunxi_efuse_read(key_map->name, key_data, key_map->size);
	if (key_map->size != (read_size << 3)) {
		printf("read len %d error, should be %d\n!", read_size, key_map->size);
		ret = -1;
	}

	printf("efuse [%s] data: \n", key_map->name);
	hexdump(key_data, key_map->size / 8);

	free(key_data);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_tfm_efuse_read, tfm_efuse_read, tfm efuse read);
#endif
