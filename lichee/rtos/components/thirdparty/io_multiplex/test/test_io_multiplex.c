#include <stdio.h>
#include <io_multi_poll.h>
#include <io_multi_select.h>
#include <pthread.h>
#include <fcntl.h>

static void* test_select_main(void *args)
{
    fd_set rd;
    struct timeval tv;
    int err;
    int count = 0;
    _uart_config_t uart_config;

	uart_config.baudrate = UART_BAUDRATE_115200;
	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_1;
	uart_config.parity = UART_PARITY_NONE;

    int fd = open("/dev/uart0", O_WRONLY);
    hal_uart_control(2, 0, &uart_config);
	hal_uart_disable_flowcontrol(2);

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    while(1)
    {
	FD_ZERO(&rd);
        FD_SET(fd, &rd);

        err = select(fd + 1, &rd, NULL, NULL, &tv);

        if (err == 0)
        {
            printf("select time out!, %s\n", (char *)args);
        }
        else if (err == -1)
        {
            printf("fail to select!, %s\n", (char *)args);
        }
        else
        {
            printf("data is available!, %s\n", (char *)args);
            char c;
            read(fd, &c, 1);
            printf("0x%X, ", c);
            count++;
            if(c == 'q')
            {
                break;
            }
        }
    }
    printf("\r\n");

    close(fd);
    return NULL;
}

static int cmd_test_select(int argc, const char **argv)
{
    pthread_t tid;
    pthread_attr_t   attr;
    struct sched_param sched;

    memset(&sched, 0, sizeof(sched));
    sched.sched_priority = 21;
    pthread_attr_init(&attr);
    pthread_attr_setschedparam(&attr, &sched);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    char *name = "tid";

    if ((pthread_create(&tid, &attr, test_select_main, (void *)name)) == -1)
    {
        printf("create error!\n");
        return 1;
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_test_select, __cmd_test_select, slect test);

static int cmd_test_poll(int argc, const char **argv)
{
    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_test_poll, __cmd_test_poll, poll test);
