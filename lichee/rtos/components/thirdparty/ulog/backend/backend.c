#include <ulog.h>
#include <syslog.h>
#include <console.h>
#include <fcntl.h>

int ulog_console_backend_init(void);
int ulog_file_backend_init(void);

static int ulog_open = 0;

int ulog_backend_init(void)
{
    ulog_init();

#ifdef CONFIG_ULOG_USING_SYSLOG
    openlog(CONFIG_ULOG_TAG, 0, 0);
#endif

#ifdef CONFIG_ULOG_BACKEND_USING_FILESYSTEM
    ulog_file_backend_init();
#endif

#ifdef CONFIG_ULOG_BACKEND_USING_CONSOLE
    ulog_console_backend_init();
#endif

    ulog_open = 1;
}

__attribute__((weak)) void show_logfile_info(void)
{
    printf("No log file!\n");
}

__attribute__((weak)) void syslog( int priority, const char * message, ...)
{
    va_list args;

    va_start(args, message);

    log_i(message);

    va_end(args);
}

#ifdef CONFIG_COMMAND_SYSLOG_TEST
int cmd_syslog(int argc, char ** argv)
{
    if(ulog_open == 0)
    {
        ulog_backend_init();
    }

    if(argc < 2)
    {
        show_logfile_info();
        return -1;
    }

    if(argc == 2)
    {
        syslog(LOG_USER, "%s\n", argv[1]);
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_syslog, syslog, Put a log to syslog system);
#endif
