#include <stdio.h>
#include <string.h>

#include <sunxi_hal_common.h>
#include <hal_interrupt.h>
#include <hal_cmd.h>

#define VIRT_LOG_SIZE				(CONFIG_COMPONENTS_VIRT_LOG_SIZE * 1024)

static char virt_log_buffer[VIRT_LOG_SIZE];
static uint32_t log_offset = 0;
static uint8_t log_enable = 1;
static uint8_t virt_log_output_enable = 0;
static uint32_t output_dirty_offset = 0;
static uint32_t buffer_full_cnt = 0;
static uint32_t buffer_full_cnt_save = 0;

void virt_log_put(char ch)
{
	unsigned long flags;

	/* currently only the sigle-core case is considered */
	flags = hal_interrupt_disable_irqsave();
	if (!log_enable) {
		hal_interrupt_enable_irqrestore(flags);
		return;
	}

	virt_log_buffer[log_offset] = ch;
	log_offset++;
	if (log_offset >= VIRT_LOG_SIZE) {
		log_offset = 0;
		buffer_full_cnt++;
	}
	hal_interrupt_enable_irqrestore(flags);
}

int virt_log_put_buf(char *buf)
{
	int len, remain, writed;
	unsigned long flags;

	flags = hal_interrupt_disable_irqsave();
	if (!log_enable) {
		hal_interrupt_enable_irqrestore(flags);
		return 0;
	}

	len = strlen(buf);
	if (len > VIRT_LOG_SIZE) {
		buf += (len - VIRT_LOG_SIZE);
		len -= (len - VIRT_LOG_SIZE);
	}
	writed = len;

	/* currently only the sigle-core case is considered */
	remain = VIRT_LOG_SIZE - log_offset;
	if (len > remain) {
		memcpy(&virt_log_buffer[log_offset], buf, remain);
		log_offset = 0;
		len -= remain;
		buf += remain;
	}
	memcpy(&virt_log_buffer[log_offset], buf, len);
	log_offset += len;
	if (log_offset >= VIRT_LOG_SIZE)
		log_offset = 0;
	hal_interrupt_enable_irqrestore(flags);

	return writed;
}

int virt_log_put_buf_len(char *buf, int len)
{
	int remain, writed;
	unsigned long flags;

	flags = hal_interrupt_disable_irqsave();

	if (!log_enable) {
		hal_interrupt_enable_irqrestore(flags);
		return 0;
	}

	if (len > VIRT_LOG_SIZE) {
		buf += (len - VIRT_LOG_SIZE);
		len -= (len - VIRT_LOG_SIZE);
	}
	writed = len;

	/* currently only the sigle-core case is considered */
	remain = VIRT_LOG_SIZE - log_offset;
	if (len > remain) {
		memcpy(&virt_log_buffer[log_offset], buf, remain);
		log_offset = 0;
		buffer_full_cnt++;
		len -= remain;
		buf += remain;
	}
	memcpy(&virt_log_buffer[log_offset], buf, len);
	log_offset += len;
	if (log_offset >= VIRT_LOG_SIZE) {
		log_offset = 0;
		buffer_full_cnt++;
	}
	hal_interrupt_enable_irqrestore(flags);

	return writed;
}

int virt_log_flush(void)
{
	uint32_t offset, i;
	unsigned long flags;

	flags = hal_interrupt_disable_irqsave();
	log_enable = 0;
	offset = log_offset;

	if (buffer_full_cnt == buffer_full_cnt_save) {
		for (i = output_dirty_offset; i < offset; i++)
			putchar(virt_log_buffer[i]);
	} else if ((buffer_full_cnt - buffer_full_cnt_save) > 1) {
		for (i = offset; i < VIRT_LOG_SIZE; i++)
			putchar(virt_log_buffer[i]);
		for (i = 0; i < offset; i++)
			putchar(virt_log_buffer[i]);
	} else {
		if (output_dirty_offset >= offset) {
			for (i = output_dirty_offset; i < VIRT_LOG_SIZE; i++)
				putchar(virt_log_buffer[i]);
			for (i = 0; i < offset; i++)
				putchar(virt_log_buffer[i]);
		} else {
			for (i = offset; i < VIRT_LOG_SIZE; i++)
				putchar(virt_log_buffer[i]);
			for (i = 0; i < offset; i++)
				putchar(virt_log_buffer[i]);
		}
	}

	output_dirty_offset = offset;
	buffer_full_cnt_save = buffer_full_cnt;
	log_enable = 1;
	hal_interrupt_enable_irqrestore(flags);

	return 0;
}

int virt_log_is_enable(void)
{
    return virt_log_output_enable;
}

void virt_log_enable(int enable)
{
	unsigned long flags;

	flags = hal_interrupt_disable_irqsave();
    virt_log_output_enable = enable;
	if (enable == 0)
		virt_log_flush();
	hal_interrupt_enable_irqrestore(flags);
}

#ifdef CONFIG_COMMAND_DMESG
static int cmd_dmesg(int argc, const char **argv)
{
	virt_log_flush();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_dmesg, dmesg, dump log info);
#endif
