#include <stdio.h>
#include <tinatest.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <console.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <tinatest.h>
#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <spiffs.h>
#include <blkpart.h>
#include <sys/vfs.h>
#include <awlog.h>
#include <sys/time.h>
#include <stdarg.h>

/* for fpga debug */
#define NO_GETTIMEOFDAY

#ifdef NO_GETTIMEOFDAY

#ifndef OS_MSEC_PER_SEC
#define OS_MSEC_PER_SEC 1000U
#endif

#ifndef OS_HZ
#define OS_HZ               CONFIG_HZ//configTICK_RATE_HZ
#endif

#ifndef OS_TICK
#define OS_TICK configTICK_RATE_HZ
#endif

#ifndef OS_TicksToMSecs
#define OS_TicksToMSecs(t) ((uint32_t)(t) / (OS_MSEC_PER_SEC / OS_TICK))
#endif

#ifndef OS_GetTicks
#define OS_GetTicks()       (xTaskGetTickCount())
#endif

#ifndef OS_GetTime
#define OS_GetTime()        (OS_GetTicks() / OS_HZ)
#endif

#endif

#define KB ((unsigned long long)1024)
#define MB (KB * 1024)
#define GB (KB * 1024)

#define DEFAULT_RUN_TIMES 3
#define DEFAULT_ORG "rwspeed.tmp"
#define DEFAULT_BUF_SIZE (4 * 1024)

#define VERSION "v0.1.0"
#define COMPILE "Compiled in " __DATE__ " at " __TIME__

struct rwtask {
    char *file;
    char *dir;
    char *path;
    unsigned int avg;
    unsigned long long size;
    unsigned long long r_ms;
    unsigned long long w_ms;

    struct sys {
        uint32_t free_ddr;
        uint32_t total_ddr;
        uint32_t total_flash;
    } sys;
};

static unsigned long long str_to_bytes(const char *str)
{
    unsigned long long size;
    char c;

    c = str[strlen(str) - 1];
    size = atoll(str);
    if (size == 0)
        return 0;

    switch (c) {
    case '0'...'9': return size;
    case 'k':
    case 'K': return size * KB;
    case 'm':
    case 'M': return size * MB;
    case 'g':
    case 'G': return size * GB;
    default:
          return 0;
    }
}

static int randbuf(char *buf, int len)
{
    int x;

    srand((unsigned int)time(NULL));
    while (len) {
        int l = min(len, (int)sizeof(int));

        x = rand();
        memcpy(buf, &x, l);

        len -= l;
        buf += l;
    }
    return 0;
}

static inline uint32_t get_free_ddr(void)
{
    return xPortGetFreeHeapSize();
}

static inline uint32_t get_total_ddr(void)
{
    extern int _heap_start, _heap_end;

    return ((unsigned long)&_heap_end - (unsigned long)&_heap_start) << 2;
}

static inline uint32_t get_total_flash(const char *dir)
{
    struct statfs sfs;

    if (!dir)
        return 0;

    if (statfs(dir, &sfs) < 0) {
        printf("statfs %s failed\n", dir);
        return 0;
    }

    return sfs.f_bsize * sfs.f_blocks;
}

static inline uint32_t get_free_flash(const char *dir)
{
    struct statfs sfs;

    if (!dir)
        return 0;

    if (statfs(dir, &sfs) < 0)
        return 0;

    /* get free by f_bavail rather than f_bfree */
    return sfs.f_bsize * sfs.f_bavail;
}

static inline uint32_t get_file_size(const char *file)
{
    struct stat s;
    int ret;

    ret = stat(file, &s);
    if (ret)
        return 0;
    if (!S_ISREG(s.st_mode))
        return 0;
    return s.st_size;
}

static void show_help(void)
{
    printf("    Usage: rwspeed <-d dir> [-h] [-t avg] [-s size]\n");
    printf("\n");
    printf("\t-h : show this massage and exit\n");
    printf("\t-d : # : the diretory to check [default currect path]\n");
    printf("\t-t # : check times for average\n");
    printf("\t-s # : set file size\n");

    printf("\n");
    printf("  size trailing with k|m|g or not\n");
}

static void print_head(struct rwtask *task)
{
    time_t t = time(NULL);
    struct sys *sys = &task->sys;

    printf("\n\trwspeed: get seq read/write speed\n\n");
    printf("\tversion: %s\n", VERSION);
    printf("\tbuild: %s\n", COMPILE);
    printf("\tdate: %s\n", ctime(&t));
    printf("\tfree/total ddr: %llu/%llu KB\n",
            sys->free_ddr / KB, sys->total_ddr / KB);
    if (task->dir)
        printf("\tfree/total flash: %llu/%llu KB\n",
                get_free_flash(task->dir) / KB, sys->total_flash / KB);
    else
        printf("\tfile size: %u KB\n", get_file_size(task->file));
    printf("\tset file size to %lld KB\n", task->size / KB);
    printf("\tset average to %u\n", task->avg);
    if (task->file)
        printf("\tset check file as %s\n", task->file);
    else
        printf("\tset check diretory as %s\n", task->dir);
    printf("\tset r/w file as %s\n", task->path);
}

static void print_end(struct rwtask *task)
{

    printf("\n");
    printf("\tread average speed: %.2f KB/s\n",
            ((double)task->size * task->avg / KB) / ((double)task->r_ms / 1000));
    printf("\twrite average speed: %.2f KB/s\n",
            ((double)task->size * task->avg / KB) / ((double)task->w_ms / 1000));

#ifdef AW_TINA_TEST
    ttips("read %.2f KB/s write %.2f KB/s\n",
            ((double)task->size * task->avg / KB) / ((double)task->r_ms / 1000),
            ((double)task->size * task->avg / KB) / ((double)task->w_ms / 1000));
#endif
}

static int do_remove(struct rwtask *task)
{
    int ret = 0;

    if (task->dir) {
        ret = unlink(task->path);
        if (!ret)
            printf("\tremove\t: %s ... OK\n", task->path);
        else
            printf("\tremove\t: %s ... Failed - %s\n", task->path, strerror(errno));
    }
    return ret;
}

static inline unsigned long long auto_size(struct rwtask *task)
{
    if (task->dir)
        return get_free_flash(task->dir) * 85 / 100;
    else
        return get_file_size(task->file);
}

static int do_write(struct rwtask *task)
{
    int fd, ret = -1;
    char *buf;
    unsigned long long size = 0, start_ms = 0, end_ms = 0, ms = 0;
    struct timeval start = {0}, end = {0};

    printf("\r\twrite\t: %s ... ", task->path);

    buf = malloc(DEFAULT_BUF_SIZE);
    if (!buf) {
        printf("malloc failed - %s\n", strerror(errno));
        return -ENOMEM;
    }
    randbuf(buf, DEFAULT_BUF_SIZE);

    fd = open(task->path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        printf("open failed - %s\n", strerror(errno));
        goto out;
    }

    ret = gettimeofday(&start, NULL);
    if (ret) {
        printf("gettimeofday failed - %s\n", strerror(errno));
        goto out;
    }
#ifdef NO_GETTIMEOFDAY
    start_ms = OS_TicksToMSecs(OS_GetTicks());
#endif

    ret = -1;
    size = task->size;
    while (size > 0) {
        unsigned long long done;
        unsigned long long sz;

        sz = min(size, (unsigned long long)DEFAULT_BUF_SIZE);
        done = write(fd, buf, sz);
        if (done != sz) {
            printf("write failed - %s\n", strerror(errno));
            close(fd);
            goto out;
        }
        size -= sz;
    }
    fsync(fd);
    close(fd);

#ifdef NO_GETTIMEOFDAY
    end_ms = OS_TicksToMSecs(OS_GetTicks());
#endif
    ret = gettimeofday(&end, NULL);
    if (ret) {
        printf("gettimeofday failed - %s\n", strerror(errno));
        goto out;
    }

    /* calculate the speed */
#ifndef NO_GETTIMEOFDAY
    start_ms = start.tv_sec * 1000 + start.tv_usec / 1000;
    end_ms = end.tv_sec * 1000 + end.tv_usec / 1000;
#endif
    ms = end_ms - start_ms;
    task->w_ms += ms;
    printf("OK (%.2f KB/s)\n", ((double)task->size / KB) / ((double)ms / 1000));

    ret = 0;
out:
    free(buf);
    return ret;
}

static int do_read(struct rwtask *task)
{
    int fd, ret = -1;
    char *buf;
    unsigned long long size = 0, start_ms = 0, end_ms = 0, ms = 0;
    struct timeval start = {0}, end = {0};

    printf("\r\tread\t: %s ... ", task->path);

    buf = malloc(DEFAULT_BUF_SIZE);
    if (!buf) {
        printf("malloc failed - %s\n", strerror(errno));
        return -ENOMEM;
    }

    fd = open(task->path, O_RDONLY, 0666);
    if (fd < 0) {
        printf("open failed - %s\n", strerror(errno));
        goto out;
    }

    ret = gettimeofday(&start, NULL);
    if (ret) {
        printf("gettimeofday failed - %s\n", strerror(errno));
        goto out;
    }

#ifdef NO_GETTIMEOFDAY
    start_ms = OS_TicksToMSecs(OS_GetTicks());
#endif
    ret = -1;
    size = task->size ? task->size : auto_size(task);
    while (size > 0) {
        unsigned long long done;
        unsigned long long sz;

        sz = min(size, (unsigned long long)DEFAULT_BUF_SIZE);
        done = read(fd, buf, sz);
        if (done != sz) {
            printf("read failed - %s\n", strerror(errno));
            close(fd);
            goto out;
        }
        size -= sz;
    }

#ifdef NO_GETTIMEOFDAY
    end_ms = OS_TicksToMSecs(OS_GetTicks());
#endif
    ret = gettimeofday(&end, NULL);
    if (ret) {
        printf("gettimeofday failed - %s\n", strerror(errno));
        goto out;
    }

    close(fd);

    /* calculate the speed */
#ifndef NO_GETTIMEOFDAY
    start_ms = start.tv_sec * 1000 + start.tv_usec / 1000;
    end_ms = end.tv_sec * 1000 + end.tv_usec / 1000;
#endif
    ms = end_ms - start_ms;
    task->r_ms += ms;
    printf("OK (%.2f KB/s)\n", ((double)task->size / KB) / ((double)ms / 1000));

    ret = 0;
out:
    free(buf);
    return ret;
}

static int begin(struct rwtask *task)
{
    int avg = task->avg;
    int ret;

    print_head(task);

    while (avg-- > 0) {
        printf("\n");

        ret = do_write(task);
        if (ret)
            return ret;

        ret = do_read(task);
        if (ret)
            return ret;

        ret = do_remove(task);
        if (ret)
            return ret;
    }

    print_end(task);

    return 0;
}

int rwspeed_main(int argc, char **argv)
{
    int opts = 0;
    int ret = 0;
    struct rwtask *task;

    printf("%s@%p\n", __func__, rwspeed_main);

    task = malloc(sizeof(struct rwtask));
    if (!task) {
        printf("malloc for task failed\n");
        return -1;
    }
    memset(task, 0, sizeof(*task));

    optind = 0;
    while ((opts = getopt(argc, argv, ":hd:t:s:f:")) != EOF) {
        switch (opts) {
        case 'h': show_help(); goto out;
        case 'f': {
                if (task->dir) {
                    printf("-f and -d is conflicting\n");
                    goto out;
                }
                task->file = optarg;
                break;
              }
        case 'd': {
                if (task->file) {
                    printf("-f and -d is conflicting\n");
                    goto out;
                }
                /* Alios's spiffs do not support stat directory */
                /*
                struct stat s;

                ret = stat(optarg, &s);
                if (ret) {
                    printf("stat %s failed - %s\n", optarg, strerror(errno));
                    goto out;
                }

                if (!S_ISDIR(s.st_mode)) {
                    printf("%s is not directory\n", optarg);
                    goto out;
                }
                */
                task->dir = optarg;
                break;
              }
        case 't': {
                task->avg = atoi(optarg);
                if (!task->avg) {
                    printf("average times %s is zero or invalid\n", optarg);
                    goto out;
                }
                break;
              }
        case 's': {
                task->size = str_to_bytes(optarg);
                if (!task->size) {
                    printf("size %s is zero or invalid\n", optarg);
                    goto out;
                }
                break;
              }
        case '?':
                printf("invalid option %c\n", optopt);
                goto out;
        case ':':
                printf("option -%c requires an argument\n", optopt);
                show_help();
                goto out;
        }
    }

    if (!task->dir && !task->file) {
        printf("Which directory/file to check? Please tell me by '-d/-f'\n");
        show_help();
        goto out;
    }

    if (!task->avg)
        task->avg = DEFAULT_RUN_TIMES;

    if (task->file) {
        task->path = task->file;
    } else {
        task->path = malloc(strlen(task->dir) + sizeof(DEFAULT_ORG) + 2);
        if (!task->path) {
            printf("malloc for path failed\n");
            goto out;
        }
        sprintf(task->path, "%s/%s", task->dir, DEFAULT_ORG);
        unlink(task->path);
    }

    if (!task->size)
        task->size = auto_size(task);

    task->sys.free_ddr = get_free_ddr();
    task->sys.total_ddr = get_total_ddr();
    task->sys.total_flash = get_total_flash(task->dir);

    ret = begin(task);
    free(task->path);
out:
    free(task);
    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(rwspeed_main, rwspeed, get seq read/write speed);

#ifdef AW_TINA_TEST
int rwspeed_tt(int argc, char **argv)
{
    char *_argv[5];
    int i = 0, ret;

    _argv[i++] = "rwspeed";
    _argv[i++] = "-s";
    _argv[i++] = "128k";
    _argv[i++] = "-d";
    if (argc > 1)
        _argv[i++] = argv[1];
    else
        _argv[i++] = "/data";

    ret = rwspeed_main(i, _argv);
    if (ret)
        printf("rwspeed failed\n");

    return ret;
}
testcase_init(rwspeed_tt, rwspeed, get seq read/write speed);
#endif
