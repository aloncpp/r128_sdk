
#include "console.h"
#include "cmd_util.h"
#include "log_core.h"
#include "log_core_inner.h"
#include "log_parse_cmd.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

static int (*extern_parase_function)(char *key, char *op, char *val) = NULL;

#define DEFINE_TESET2(log_struct, key, op, val)                                                    \
    {                                                                                              \
        LOG_DEBUG("key = %s,op='%s',val=%s", key, op, val);                                        \
        if (strcmp(op, "+=") == 0) {                                                               \
            if (strcmp(key, "type") == 0) {                                                        \
                log_struct.type |= (log_hub_type_t)strtol(val, NULL, 0);                           \
            } else if (strcmp(key, "setting") == 0) {                                              \
                log_struct.setting |= (int)strtol(val, NULL, 0);                                   \
            } else if (strcmp(key, "level") == 0) {                                                \
                log_struct.level |= (log_level_t)strtol(val, NULL, 0);                             \
                LOG_DEBUG("level = %ld\n", strtol(val, NULL, 0));                                  \
            } else {                                                                               \
                LOG_ERROR("error key = %s", key);                                                  \
            }                                                                                      \
        } else if (strcmp(op, "-=") == 0) {                                                        \
            if (strcmp(key, "type") == 0) {                                                        \
                log_struct.type &= ~(log_hub_type_t)strtol(val, NULL, 0);                          \
            } else if (strcmp(key, "setting") == 0) {                                              \
                log_struct.setting &= ~(int)strtol(val, NULL, 0);                                  \
            } else if (strcmp(key, "level") == 0) {                                                \
                log_struct.level &= ~(log_level_t)strtol(val, NULL, 0);                            \
                LOG_DEBUG("level = %ld\n", strtol(val, NULL, 0));                                  \
            } else {                                                                               \
                LOG_ERROR("error key = %s", key);                                                  \
            }                                                                                      \
        } else if (strcmp(op, "=") == 0) {                                                         \
            if (strcmp(key, "type") == 0) {                                                        \
                log_struct.type = (log_hub_type_t)strtol(val, NULL, 0);                            \
            } else if (strcmp(key, "setting") == 0) {                                              \
                log_struct.setting = (int)strtol(val, NULL, 0);                                    \
            } else if (strcmp(key, "level") == 0) {                                                \
                log_struct.level = (log_level_t)strtol(val, NULL, 0);                              \
                LOG_DEBUG("level = %ld\n", strtol(val, NULL, 0));                                  \
            } else {                                                                               \
                LOG_ERROR("error key = %s", key);                                                  \
            }                                                                                      \
        } else {                                                                                   \
            LOG_ERROR("errror operator = %s", op);                                                 \
        }                                                                                          \
    }

static char *strdelspace(char *in)
{
    char *out = NULL;
    char *p = in;
    while ((*p == ' ') || (*p == '\t'))
        p++;
    out = p;
    while (1) {
        if (*p == ' ')
            break;
        if (*p == '\n')
            break;
        if (*p == '\0')
            break;
        if (*p == '\t')
            break;
        p++;
    }
    *p = '\0';
    return out;
}

static void log_process_cmdline(char *input_str, uint32_t len)
{
    // 	echo "type = LOG_LEVEL_WARNING"> debug
    // 	echo "level = 5"> debug
    // 	echo "setting+=LOG_SETTING_FFLUSH" > debug
    // echo 5 > debug
    do {
        if (len > 64) {
            return;
        }
        if (input_str[0] == '\n') {
            return;
        }
        if (len == 2) {
            int debug_level = atoi(input_str);
            if (debug_level >= 0 && debug_level <= 5) {
                global_log_type.level = (log_level_t)debug_level;
                return;
            } else {
                break;
            }
        }
    } while (0);

    static char key[64] = { 0 };
    static char val[64] = { 0 };
    memset(key, 0, sizeof(key));
    memset(val, 0, sizeof(val));
    char op[3] = { 0 };

    if (strstr(input_str, "+=")) {
        sscanf(input_str, "%30[0-9a-zA-Z\t ]+=%30s", key, val);
        strcpy(op, "+=");
    } else if (strstr(input_str, "-=")) {
        sscanf(input_str, "%30[0-9a-zA-Z\t ]-=%30s", key, val);
        strcpy(op, "-=");
    } else if (strstr(input_str, "=")) {
        sscanf(input_str, "%30[0-9a-zA-Z\t ]=%30s", key, val);
        strcpy(op, "=");
    }
    char *k = strdelspace(key);
    char *v = strdelspace(val);
    LOG_DEBUG("log_core: %s, key = %s, val = %s\n", input_str, k, v);

    DEFINE_TESET2(global_log_type, k, op, v);
    if (extern_parase_function) {
        extern_parase_function(k, op, v);
    }
    return;
}

// log_process_cmdline(buf, strlen(buf) + 1);

int log_parse_cmd_start(char *fifo_path)
{
    return 0;
}

int log_parse_cmd_stop(void)
{
    return 0;
}

/**
 * @input: callback_function
 * @param: echo a1 +=  1b > logcore,[key = a1, op = '+=' val=1b]
 *         op in ['=','+=','-=']  
 **/
int log_parse_cmd_register_function(int (*fun)(char *key, char*op, char*val))
{
    extern_parase_function = fun;
    return 0;
}

static enum cmd_status log_parse_cmd_secondary_exec(char *cmd)
{
    if (strlen(cmd) > 64) {
        return CMD_STATUS_FAIL;
    }
    log_process_cmdline(cmd, strlen(cmd));
    return CMD_STATUS_OK;
}

static void log_parse_cmd_exec(int argc, char *argv[])
{
    //cmd2_main_exec(argc, argv, log_parse_cmd_secondary_exec);
}

FINSH_FUNCTION_EXPORT_CMD(log_parse_cmd_exec, wlog, log testcmd);

