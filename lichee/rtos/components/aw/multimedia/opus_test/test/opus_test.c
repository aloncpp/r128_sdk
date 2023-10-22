#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <console.h>
extern int opus_encode_demo(void);
extern int opus_decode_demo(void);

int cmd_test_opus(void)
{

	printf("opus encode and decode has large array, please pay attention to stack size.\n");

	printf("start encode demo\n");
	opus_encode_demo();

	printf("start decode demo\n");
	opus_decode_demo();

	printf("finish encode and decode\n");

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_opus, test_opus, test opus);
