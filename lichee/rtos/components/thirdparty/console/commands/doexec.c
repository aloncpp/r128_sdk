#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sunxi_hal_common.h>

#include <console.h>

#define MAX_BUFFER_SIZE 1024

portBASE_TYPE prvCommandEntry( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

static void show_usage(void)
{
	printf("Usage: exec filename\r\n");
}

static int cmd_exec(int argc, char **argv)
{
	char *filename = NULL;
	FILE *file = NULL;
	unsigned char *buffer = NULL;

	if (argc < 2) {
		show_usage();
		return -1;
	}

	buffer = hal_malloc_coherent(MAX_BUFFER_SIZE);
	if (!buffer) {
		printf("alloc memory failed!\r\n");
		return -1;
	}

	filename = argv[1];

	file = fopen(filename, "r");
	if (file == NULL) {
		printf("open %s failed!\r\n", filename);
		hal_free_coherent(file);
		return -1;
	}

	memset(buffer, 0, MAX_BUFFER_SIZE);

	while(fgets(buffer, MAX_BUFFER_SIZE - 1, file)) {
		buffer[strlen(buffer) - 1] = 0;
		prvCommandEntry(NULL, 0, buffer);
		memset(buffer, 0, MAX_BUFFER_SIZE);
	}

	fclose(file);
	hal_free_coherent(file);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_exec, exec, exec file);
