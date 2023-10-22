#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hal_gpio.h>
#include <hal_uart.h>
#include "platform_bsp.h"

#define BT_RST_PIN GPIO_PE4
#define AP_WAKE_BT_PIN GPIO_PE3
#define UART_PORT UART_1

void xr_bt_rf_init(void)
{
	hal_gpio_set_pull(BT_RST_PIN, GPIO_PULL_UP);
	hal_gpio_set_direction(BT_RST_PIN, GPIO_DIRECTION_OUTPUT);
	hal_gpio_set_data(BT_RST_PIN, GPIO_DATA_LOW);

	hal_gpio_set_pull(AP_WAKE_BT_PIN, GPIO_PULL_UP);
	hal_gpio_set_direction(AP_WAKE_BT_PIN, GPIO_DIRECTION_OUTPUT);
	hal_gpio_set_data(AP_WAKE_BT_PIN, GPIO_DATA_LOW);
}

void xr_bt_rf_reset(void)
{
	hal_gpio_set_data(AP_WAKE_BT_PIN, GPIO_DATA_HIGH);
	usleep(10 * 1000);
	hal_gpio_set_data(BT_RST_PIN, GPIO_DATA_LOW);
	usleep(10 * 1000);
	hal_gpio_set_data(BT_RST_PIN, GPIO_DATA_HIGH);
}

void xr_bt_rf_deinit(void)
{
	hal_gpio_set_data(BT_RST_PIN, GPIO_DATA_LOW);
	usleep(10 * 1000);
	hal_gpio_set_data(AP_WAKE_BT_PIN, GPIO_DATA_LOW);
}

int xr_uart_init(void)
{
	int ret = 0;
	_uart_config_t uart_config;

	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_1;
	uart_config.parity = UART_PARITY_NONE;
	uart_config.baudrate = UART_BAUDRATE_115200;

	ret = hal_uart_init(UART_PORT);
	if (ret < 0) {
		printf("uart_init fail.\n");
		return ret;
	}

	ret = hal_uart_control(UART_PORT, 0, &uart_config);
	if (ret < 0) {
		printf("uart config fail.\n");
		return ret;
	}

	return ret;
}

void xr_uart_disable_flowcontrol(void)
{
	hal_uart_disable_flowcontrol(UART_PORT);
}

void xr_uart_enable_flowcontrol(void)
{
	hal_uart_set_hardware_flowcontrol(UART_PORT);
}

int xr_uart_set_baudrate(uint32_t baudrate)
{
	_uart_config_t uart_config;
	uart_baudrate_t _baudrate;
	switch(baudrate) {
		case 9600:
			_baudrate = UART_BAUDRATE_9600;
			break;
		case 115200:
			_baudrate = UART_BAUDRATE_115200;
			break;
		case 1500000:
			_baudrate = UART_BAUDRATE_1500000;
			break;
		default:
			break;
	}
	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_1;
	uart_config.parity = UART_PARITY_NONE;
	uart_config.baudrate = _baudrate;

	return hal_uart_control(UART_PORT, 0, &uart_config);
}

int xr_uart_send_data(uint8_t *buf, uint32_t count)
{
	return hal_uart_send(UART_PORT, buf, count);
}

int xr_uart_receive_data(uint8_t *buf, uint32_t count)
{
	return hal_uart_receive(UART_PORT, buf, count);
}

int xr_uart_receive_data_no_block(uint8_t *buf,  uint32_t count, int32_t timeout)
{
	return hal_uart_receive_no_block(UART_PORT, buf, count, timeout);
}

void xr_uart_set_loopback(bool enable)
{
	hal_uart_set_loopback(UART_PORT, enable);
}

int xr_uart_deinit(void)
{
	return hal_uart_deinit(UART_PORT);
}

static const struct xr_bluetooth_rf xr_bt_rf = {
	.init = xr_bt_rf_init,
	.reset = xr_bt_rf_reset,
	.deinit = xr_bt_rf_deinit,
};

static const struct xr_bluetooth_uart xr_bt_uart = {
	.init                   = xr_uart_init,
	.disable_flowcontrol    = xr_uart_disable_flowcontrol,
	.enable_flowcontrol     = xr_uart_enable_flowcontrol,
	.send_data              = xr_uart_send_data,
	.receive_data           = xr_uart_receive_data,
	.receive_data_no_block  = xr_uart_receive_data_no_block,
	.set_baudrate           = xr_uart_set_baudrate,
	.set_loopback           = xr_uart_set_loopback,
	.deinit                 = xr_uart_deinit,
};

const struct xr_bluetooth_rf *xradio_get_platform_rf(void)
{
	return &xr_bt_rf;
}

const struct xr_bluetooth_uart *xradio_get_platform_uart(void)
{
	return &xr_bt_uart;
}
