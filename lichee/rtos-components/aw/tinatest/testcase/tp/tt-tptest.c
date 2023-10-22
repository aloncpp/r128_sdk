#include <FreeRTOS.h>
#include <stdint.h>
#include <string.h>
#include <tinatest.h>
#include <stdio.h>
#include "sunxi-input.h"

extern int gt911_init(void);

#define GT911_DEV_NAME	"touchscreen"

void tt_tp_test_task(void)
{
	int fd;
	int x = -1, y = -1, key_count = 0;
	struct sunxi_input_event event;
	memset(&event, 0, sizeof(struct sunxi_input_event));

	fd = sunxi_input_open(GT911_DEV_NAME);

	if (fd < 0) {
		printf("gpio key open err\n");
		vTaskDelete(NULL);
		return;
	}

	while(key_count < 400) {
		sunxi_input_read(fd, &event, sizeof(struct sunxi_input_event));
	//	printf("read event %d %d %d", event.type, event.code, event.value);

		if (event.type == EV_ABS) {
			switch (event.code) {
				case ABS_MT_POSITION_X:
					x = event.value;
					break;
				case ABS_MT_POSITION_Y:
					y = event.value;
					break;
			}
		} else if (event.type == EV_KEY) {
			if (event.code == BTN_TOUCH && event.value == 0)
				printf("event.code == BTN_TOUCH, value = %d\n", event.value);
		}
		if (x >= 0 && y >= 0) {
			printf("====press (%d, %d)====\n", x, y);
			x = -1;
			y = -1;
			key_count ++;
		}
	}
	printf("=======tptest successful!========\n");
	vTaskDelete(NULL);
}

int tt_tptest(int argc, char **argv)
{
	int ret = -1;

	ret = gt911_init();

	if (ret < 0)
		printf("gt911 init fail\n");

	portBASE_TYPE task_ret;
	task_ret = xTaskCreate(tt_tp_test_task, (signed portCHAR *) "tp_test_task", 1024, NULL, 0, NULL);

	return 0;
}

testcase_init(tt_tptest, tptest, tptest for tinatest);
