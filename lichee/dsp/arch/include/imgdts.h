#ifndef __IMGDTS_H
#define __IMGDTS_H

#define DTS_CLOSE (0)
#define DTS_OPEN  (1)

struct dts_uart_pin_msg_t {
	unsigned int port;
	unsigned int port_num;
	unsigned int mul_sel;
};

struct dts_uart_msg_t {
	unsigned int status;
	unsigned int uart_port;
	struct dts_uart_pin_msg_t uart_pin_msg[2];
};

struct dts_gpio_int_t {
	unsigned int gpio_a;
	unsigned int gpio_b;
	unsigned int gpio_c;
	unsigned int gpio_d;
	unsigned int gpio_e;
	unsigned int gpio_f;
	unsigned int gpio_g;
};

struct dts_sharespace_t {
	/* sharespace status */
	unsigned int status;
	/* dsp write space msg */
	unsigned int dsp_write_addr;
	unsigned int dsp_write_size;
	/* arm write space msg */
	unsigned int arm_write_addr;
	unsigned int arm_write_size;
	/* dsp log space msg */
	unsigned int dsp_log_addr;
	unsigned int dsp_log_size;
};

/* dts msg about dsp */
struct dts_msg_t {
	/* dsp status */
	unsigned int dsp_status;
	/* uart */
	struct dts_uart_msg_t uart_msg;
	/* gpio int */
	struct dts_gpio_int_t gpio_int;
	/* shacespace */
	struct dts_sharespace_t dts_sharespace;
};
#endif

