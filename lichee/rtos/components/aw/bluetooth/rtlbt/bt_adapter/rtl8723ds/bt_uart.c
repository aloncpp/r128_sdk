#include <stdio.h>
#include <bt_uart.h>
//#include <hci_uart.h>
#include "drivers/hal_uart.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "console.h"


static int tx_cmplt_flag = 0;

static SemaphoreHandle_t xSemaphore = NULL;

static HCI_UART_CB rx_ind = NULL;

#define MAX_SEM_CNT 10
#define RING_BUFF_SIZE (1024*8)

static char uart_buff[RING_BUFF_SIZE];

static void hex_dump(const char *pref, int width, unsigned char *buf, int len)
{
       int i,n;
    for (i = 0, n = 1; i < len; i++, n++){
        if (n == 1)
            printf("%s ", pref);
        printf("%2.2X ", buf[i]);
        if (n == width) {
            printf("\n");
            n = 0;
        }
    }
    if (i && n!=1)
        printf("\n");
}


void set_rx_ind_callback(HCI_UART_CB ind)
{
	rx_ind = ind;
}

static void bt_uart_callback(uart_callback_event_t event, void *user_data)
{
	BaseType_t ret;
	BaseType_t taskwoken = pdFALSE;
	switch(event) {
	case UART_EVENT_TX_COMPLETE:
		tx_cmplt_flag = 1;
		break;
	case UART_EVENT_RX_COMPLETE:
		ret = xSemaphoreGiveFromISR(xSemaphore, &taskwoken);
		if (ret == pdPASS) {
			portYIELD_FROM_ISR(taskwoken);
		}
		break;
	default:
		break;
	}
}

static bool tx_log_enable = false;
void bt_tx_log_enale()
{
	if(tx_log_enable == false)
		tx_log_enable = true;
	else
		tx_log_enable = false;
}
FINSH_FUNCTION_EXPORT_CMD(bt_tx_log_enale, bttx, Console bt tx log Command);

bool hci_uart_tx(uint8_t *p_buf, uint16_t len, HCI_UART_CB tx_cb)
{
	if(tx_log_enable)
		hex_dump(__func__,20,p_buf,len);
#if 1
	const TickType_t xVeryShortDelay = 2UL;


	hal_uart_send(UART_1, p_buf, len);

	while (tx_cmplt_flag == 0)
		vTaskDelay( xVeryShortDelay );

	tx_cmplt_flag = 0;
#else
	hal_uart_send_polling(UART_1, p_buf, len);
#endif
	if(tx_cb)
		tx_cb();
}
bool rx_log_enable = false;

void bt_rx_log_enale()
{
	if(rx_log_enable == false)
		rx_log_enable = true;
	else
		rx_log_enable = false;
}
FINSH_FUNCTION_EXPORT_CMD(bt_rx_log_enale, btrx, Console bt rx log Command);

#ifdef USE_GET_CHAR_BLOCK
uint8_t hci_get_char_block(uint8_t *chr , uint32_t wait_tick)
{
	static int i = 0;
#if 1
	char data ;
	hal_uart_status_t ret;
	BaseType_t xResult;

	ret = hal_uart_receive(UART_1, &data, 1);
	while(ret != HAL_UART_STATUS_OK) {
		xResult = xSemaphoreTake(xSemaphore, portMAX_DELAY);
		ret = hal_uart_receive(UART_1,&data,1);
	}

	*chr = data;
#else
	*chr = hal_uart_get_char(UART_1);
#endif

	if(rx_log_enable) {
		printf("%2.2X ",*chr);
		i++;
		if(i>20) {
			printf("\n");
			i = 0;
		}
	}
	return 0;
}
#else
void hci_rx_signal(void *arg)
{
	while(1) {
		BaseType_t xResult;
		xResult = xSemaphoreTake(xSemaphore, portMAX_DELAY);
		if(hal_uart_ring_buff_is_empty(UART_1) == false) {
			rx_ind();
		}
	}
	vTaskDelete(NULL);
}

uint16_t hci_tp5_recv(uint8_t **data, uint16_t *num)
{
	uint32_t ret;
	ret = hal_uart_receive_without_cpy(UART_1,data,num);

	if(rx_log_enable) {
		printf("%s:%d, %d, %d\n",__func__,__LINE__,ret,*num);
		//hex_dump(__func__,20,*data,*num);
	}
	return ret;
}

#endif
void comuart_init(void)
{
	portBASE_TYPE ret;
	uart_config_t tmp_config = {
		.baudrate = UART_BAUDRATE_115200,
		.word_length = UART_WORD_LENGTH_8,
		.stop_bit = UART_STOP_BIT_1,
		.parity = UART_PARITY_EVEN
    };
	uart_ring_buf_init(UART_1, uart_buff, sizeof(uart_buff));

	hal_uart_init(UART_1, &tmp_config);

	hal_uart_set_hardware_flowcontrol(UART_1);
	hal_uart_register_callback(UART_1, bt_uart_callback, NULL);

	xSemaphore = xSemaphoreCreateCounting(MAX_SEM_CNT,0);
	if (!xSemaphore)
		printf("Error creating uart1 rx semaphore");

	ret = xTaskCreate(hci_rx_signal,
				(signed portCHAR *)"hci_rx_ind",
				1024,
				NULL,
				configTIMER_TASK_PRIORITY - 1,
				NULL);

	if (ret != pdPASS) {
		printf("Error creating uart1 rx task, status was %d\r\n", ret);
		return;
	}


}

void comuart_set_baudrate(uint32_t baudrate)
{
	uart_baudrate_t uart_bd = UART_BAUDRATE_115200;
	switch(baudrate) {
		case 9600:
			uart_bd = UART_BAUDRATE_9600;
			break;
		case 115200:
			uart_bd = UART_BAUDRATE_115200;
			break;
		case 1500000:
			uart_bd = UART_BAUDRATE_1500000;
			break;
		default:
			printf("baudrate not support.\n");
			break;
	}
	if(hal_uart_set_baudrate(UART_1, uart_bd) != HAL_UART_STATUS_OK)
		printf("set baudarate failed.\n");
}

void comuart_rx_enable(void)
{
	;
}

void comuart_rx_disable(void)
{
	;
}
