#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <ulog.h>
#include <syslog.h>
#include <console.h>

#ifdef CONFIG_ULOG_BACKEND_USING_FILESYSTEM

#if defined(ULOG_ASYNC_OUTPUT_BY_THREAD) && ULOG_ASYNC_OUTPUT_THREAD_STACK < 384
#error "The thread stack size must more than 384 when using async output by thread (ULOG_ASYNC_OUTPUT_BY_THREAD)"
#endif

#define LOG_FILE CONFIG_ULOG_FILE_PATH
#define LOG_FILE_FULL CONFIG_ULOG_FILE_PATH"_full"

static struct ulog_backend file;

int ulog_get_file_size(const char * path)
{
    FILE * file;
    int size = 0;

    file = fopen(path, "r");

    if(file == NULL)
    {
       return -1;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    if(size < 0)
    {
        fclose(file);
        return -1;
    }

    fclose(file);
    return size;
}

void ulog_file_backend_output(
        struct ulog_backend *backend,
        uint32_t level,
        const char *tag,
        int8_t is_raw,
        const char *log,
        size_t len)
{
    size_t wLen = 0;
    FILE* logfile = 0;

    int logfile_size, logfile_full_size;

    logfile_size = ulog_get_file_size(LOG_FILE);
    logfile_full_size = ulog_get_file_size(LOG_FILE_FULL);

    if(logfile_size >= CONFIG_ULOG_FILE_SIZE * 1024)
    {
        remove(LOG_FILE_FULL);
        rename(LOG_FILE, LOG_FILE_FULL);
    }

    logfile = fopen(LOG_FILE, "a+");

    if(logfile)
    {
        wLen = fwrite(log, len, 1, logfile);
    }
    fclose(logfile);
}

int ulog_file_backend_init(void)
{
    file.output = ulog_file_backend_output;

    ulog_backend_register(&file, "log_file", RT_TRUE);

    return 0;
}

void show_logfile_info(void)
{
    int logfile_size, logfile_full_size;

    logfile_size = ulog_get_file_size(LOG_FILE);
    logfile_full_size = ulog_get_file_size(LOG_FILE_FULL);

    if(logfile_size <= 0)
    {
        printf("%s not exist!\n", LOG_FILE);
    }
    else
    {
        printf("%s size = %d (bytes), %d (KB)\n", LOG_FILE, logfile_size, logfile_size / 1024);
    }

    if(logfile_full_size <= 0)
    {
        printf("%s not exist!\n", LOG_FILE_FULL);
    }
    else
    {
        printf("%s size = %d (bytes), %d (KB)\n", LOG_FILE_FULL, logfile_full_size, logfile_full_size / 1024);
    }
}

#ifdef CONFIG_COMMAND_LOGGER_TEST
int cmd_logger(int argc, char ** argv)
{
    char line[128];
    char last_line[128];
    int rLen = 0;

    memset(last_line, 0, sizeof(last_line));

    FILE *file = fopen(LOG_FILE, "r");
    if(file == NULL)
    {
        return -1;
    }

    printf("\n%s:\n", LOG_FILE);
    if((argv[1] != NULL) && (!strcmp(argv[1], "-a")))
    {
        while(!feof(file))
        {
            memset(line, 0, sizeof(line));
            fgets(line, sizeof(line), file);
            printf("%s", line);
        }
    }
    else
    {
        while(!feof(file))
        {
            memset(line, 0, sizeof(line));
            fgets(line, sizeof(line), file);
            if(line[0] != EOF && line[0] != 0)
            {
                memset(last_line, 0, sizeof(last_line));
                memcpy(last_line, line, sizeof(last_line));
            }
        }
        printf("%s", last_line);
    }
    fclose(file);

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_logger, logger, Read log from syslog system);
#endif
#endif
