#include <stdio.h>
#include <hal_thread.h>
#include "multi_console_internal.h"
#include "porting.h"

#define WAIT_TIME					(1000)

#define KEY_ASCII_DEL				(0x7F)
#define KEY_ASCII_BS				(0x08)   /* backspace */

#define KEY_ASCII_SPEC1				(0x1b)
#define KEY_ASCII_SPEC2				(0x5b)

#define KEY_ASCII_FUNC_UP			(0x41)
#define KEY_ASCII_FUNC_DOWN			(0x42)
#define KEY_ASCII_FUNC_RIGHT		(0x43)
#define KEY_ASCII_FUNC_LEFT			(0x44)
#define KEY_ASCII_FUNC_HOME			(0x48)
#define KEY_ASCII_FUNC_END			(0x46)

enum input_stat {
	WAIT_NOMAL,
	WAIT_SPEC_KEY,
	WAIT_FUNC_KEY,
};

static const char *const new_line = "\r\n";
static const char *const err_line = "Undown Know Error\r\n";

void shell_thread_entry(void *param)
{
	cli_console *console = param;
	char input_str[CLI_CONSOLE_MAX_INPUT_SIZE];
	char output_str[CLI_CONSOLE_MAX_OUTPUT_SIZE];
	char read_char;
	uint16_t line_cur = 0;
	uint16_t line_pos = 0;
	enum input_stat stat = WAIT_NOMAL;
	int ret;

	memset(input_str, 0, sizeof(input_str));
	memset(output_str, 0, sizeof(output_str));

	set_current_console(console);
	console_debug("console thread:%s start.\r\n", console->name);

	if (console->dev_ops->prefix)
		cli_console_write(console, console->dev_ops->prefix, strlen(console->dev_ops->prefix));

	while (1) {
		if (console->exit)
			break;

		ret = cli_console_read_timeout(console, &read_char, sizeof(read_char), WAIT_TIME);
		if (ret < 0)
			break;
		if (ret == 0)
			continue;

		if (read_char == KEY_ASCII_SPEC1) {
			stat = WAIT_SPEC_KEY;
			continue;
		}
		if (stat == WAIT_SPEC_KEY) {
			if (read_char == KEY_ASCII_SPEC2) {
				stat = WAIT_FUNC_KEY;
				continue;
			}
			stat = WAIT_NOMAL;
		}
		if (stat == WAIT_FUNC_KEY) {
			stat = WAIT_NOMAL;

			if (read_char == KEY_ASCII_FUNC_LEFT) {
				if (line_cur) {
					cli_console_write(console, "\b", 1);
					line_cur--;
				}
				continue;
			}
			if (read_char == KEY_ASCII_FUNC_RIGHT) {
				if (line_cur < line_pos) {
					cli_console_write(console, &input_str[line_cur], 1);
					line_cur++;
				}
				continue;
			}
			if (read_char == KEY_ASCII_FUNC_HOME) {
				if (line_cur != 0) {
					int i;
					for (i = 0; i < line_cur; i++)
						cli_console_write(console, "\b", 1);
					line_cur = 0;
				}
				continue;
			}
			if (read_char == KEY_ASCII_FUNC_END) {
				if (line_cur != 0) {
					int i;
					for (i = line_cur; i <= line_pos; i++)
						cli_console_write(console, "\b", 1);
					line_cur = line_pos;
				}
				continue;
			}
		}

		if(read_char == '\n' || read_char == '\r') {
			/* empty cmd */
			if (line_pos == 0) {
				cli_console_write(console, new_line, sizeof(new_line));
				if (console->dev_ops->prefix)
					cli_console_write(console, console->dev_ops->prefix, strlen(console->dev_ops->prefix));
				continue;
			}
			if (console->dev_ops->prefix)
				cli_console_write(console, new_line, sizeof(new_line));

			input_str[line_pos] = '\0';
			ret = cli_port_parse_string(input_str, output_str, sizeof(output_str));
			if (!ret) {
				line_cur = line_pos = 0;
				cli_console_write(console, output_str, strlen(output_str));
				memset(output_str, 0, sizeof(output_str));
			} else
				cli_console_write(console, err_line, sizeof(err_line));
			if (console->dev_ops->prefix)
				cli_console_write(console, console->dev_ops->prefix, strlen(console->dev_ops->prefix));
			memset(input_str, 0, sizeof(input_str));
		} else {
			if (read_char == KEY_ASCII_BS) {
				int i;
				if (!line_cur)
					continue;
				line_cur--;
				line_pos--;
				if (line_pos > line_cur) {
					memmove(&input_str[line_cur], &input_str[line_cur + 1], line_pos - line_cur);
					input_str[line_pos] = '\0';
					/* printf("\b%s  \b") */
					cli_console_write(console, "\b", 1);
					cli_console_write(console, &input_str[line_cur], strlen(&input_str[line_cur]));
					cli_console_write(console, "  \b", sizeof("  \b"));
					/* move the cursor to thr origin position */
					for (i = line_cur; i <= line_pos; i++)
						cli_console_write(console, "\b", 1);
				} else {
					cli_console_write(console, "\b \b", sizeof("\b \b"));
					input_str[line_pos] = 0;
				}
				continue;
			} else if (read_char == KEY_ASCII_DEL) {
				int i;
				if (!line_cur)
					continue;
				if (line_pos > line_cur) {
					line_pos--;
					memmove(&input_str[line_cur], &input_str[line_cur + 1], line_pos - line_cur);
					input_str[line_pos] = '\0';
					/* printf("\b%s  \b") */
					cli_console_write(console, "\b", 1);
					cli_console_write(console, &input_str[line_cur], strlen(&input_str[line_cur]));
					cli_console_write(console, "  \b", sizeof("  \b"));
					/* move the cursor to thr origin position */
					for (i = line_cur; i <= line_pos; i++)
						cli_console_write(console, "\b", 1);
				}
				continue;
			} else {
				if((read_char >= ' ') && (read_char <= '~')) {
					if (line_pos >= sizeof(input_str))
						continue;
					if (line_cur < line_pos) {
						memmove(&input_str[line_cur + 1], &input_str[line_cur], line_pos - line_cur);
						input_str[line_cur] = read_char;
						if (console->dev_ops->echo)
							cli_console_write(console, &input_str[line_cur], strlen(&input_str[line_cur]));
					} else {
						input_str[line_pos] = read_char;
						if (console->dev_ops->echo)
							cli_console_write(console, &read_char, sizeof(read_char));
					}
					line_cur++;
					line_pos++;
				}
			}
		}
	}

	console_debug("console thread:%s exit.\r\n", console->name);
	cli_console_destory(console);
}
