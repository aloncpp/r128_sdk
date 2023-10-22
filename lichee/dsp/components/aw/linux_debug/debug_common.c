#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <console.h>
#include <xtensa/tie/xt_hifi2.h>
#include <xtensa/config/core.h>
#include <xtensa/tie/xt_timer.h>
#include <imgdts.h>
#include <components/aw/linux_debug/debug_common.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

static uint32_t SavelogMutexFlag = 0;
static SemaphoreHandle_t xSavelogMutex = NULL;
struct dts_sharespace_t dts_sharespace;
volatile uint32_t *plog;
volatile struct debug_img_t *pwrtie;

#define LOG_BUFFER_SIZE 512
static char log_buffer[LOG_BUFFER_SIZE];

void debug_common_init(void)
{
    pwrtie = (volatile struct debug_img_t *)(dts_sharespace.dsp_write_addr);
    pwrtie->log_reset = 1;
    pwrtie->log_head_addr = dts_sharespace.dsp_log_addr;
    pwrtie->log_head_size = 0;
    plog = (volatile uint32_t *)dts_sharespace.dsp_log_addr;
}

static void log_write_early(char*pstr, uint32_t size)
{
    uint32_t i = 0;
    uint32_t *pdata = (uint32_t *)pstr;
    volatile uint32_t * log_end_addr = (volatile uint32_t *)(dts_sharespace.dsp_log_addr + dts_sharespace.dsp_log_size);
    volatile uint32_t * log_start_addr = (volatile uint32_t *)dts_sharespace.dsp_log_addr;
    for (i = 0; i < (size/4); i++) {
        *plog = *pdata;
        pdata++;
        plog++;
        if (plog >= log_end_addr)
		plog = log_start_addr;
    }

    pwrtie->log_head_addr = (uint32_t)plog;
    pwrtie->log_head_size = size;
}

int log_save(const char *fmt, ...)
{

	va_list args;
	size_t length;
	uint32_t addr_check = 0;
	if (SavelogMutexFlag) {
		if (pdPASS == xSemaphoreTake(xSavelogMutex, portMAX_DELAY)){
			memset(&log_buffer, 0, LOG_BUFFER_SIZE);

			va_start(args, fmt);

			length = vsnprintf(log_buffer, sizeof(log_buffer) - 1, fmt, args);

			addr_check = length + 1;
			if ((addr_check & (~0x3U)) != addr_check)
			{
				addr_check &= (~0x3U);
				addr_check += 4;
			}

			log_write_early(log_buffer, addr_check);
			va_end(args);
			xSemaphoreGive( xSavelogMutex );
		}

	}else {

		memset(&log_buffer, 0, LOG_BUFFER_SIZE);

		va_start(args, fmt);

		length = vsnprintf(log_buffer, sizeof(log_buffer) - 1, fmt, args);

		addr_check = length + 1;
		if ((addr_check & (~0x3U)) != addr_check)
		{
			addr_check &= (~0x3U);
			addr_check += 4;
		}

		log_write_early(log_buffer, addr_check);
		va_end(args);
	}
	return 0;
}


void log_mutex_init(void)
{
	xSavelogMutex = xSemaphoreCreateMutex();
	configASSERT(xSavelogMutex);
	xSemaphoreGive(xSavelogMutex);
	SavelogMutexFlag = 1;
}


void SaveLogMutexDisable(void)
{
	SavelogMutexFlag = 0;
}


