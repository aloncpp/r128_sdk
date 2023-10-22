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
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>

#define KB ((uint64_t)1024)
#define MB (KB * 1024)
#define GB (KB * 1024)

#define DEFAULT_RUN_TIMES 1
#define DEFAULT_TEST_PERCENT 90
#define DEFAULT_TEST_SIZE (128 * KB)
#define DEFAULT_ORG_SIZE (4 * MB)
#define DEFAULT_ORG "rwcheck.org"
#define DEFAULT_TMP "rwcheck.tmp"
#define MAX_PATH_LEN (256)
#define DEFAULT_TEST_DEV "/dev/UDISK"
#define DEFAULT_TEST_DIR "/data"
#define sum_t int32_t

#define VERSION "v0.1.0"
#define COMPILE "Compiled in " __DATE__ " at " __TIME__

#define min_t(x, y) ({                \
        typeof(x) min1 = (x);        \
        typeof(y) min2 = (y);        \
        (void) ((unsigned long)&min1 == (unsigned long)&min2); \
        min1 < min2 ? min1 : min2; })
#define max_t(x, y) ({                \
        typeof(x) max1 = (x);        \
        typeof(y) max2 = (y);        \
        (void) (&max1 == &max2);    \
        max1 < max2 ? max2 : max1; })

int statfs(const char *path, struct statfs *buf);

static void show_help(void)
{
    printf("    Usage: rwcheck <-d dir> [-h] [-t times] [-s size] [-p percent]\n");
    printf("\n");
    printf("\t-h : show this massage and exit\n");
    printf("\t-d : # : the diretory to check [default currect path]\n");
    printf("\t-t # : check times\n");
    printf("\t-s # : set file size\n");
    printf("\t-p # : set maximum ratio of total flash size to check. Eg. -p 95\n");
    printf("\t-j # : set multi_thread. Eg. -j 5\n");

    printf("\n");
    printf("  size trailing with k|m|g or not\n");
}

struct rwtask
{
    const char *dir;
    unsigned int times;
    unsigned int percent;
    uint64_t size;
    uint64_t min_free;
    int task_it;
    pthread_t pth;
    pthread_barrier_t *barrier;
    uint64_t g_default_buf_size;

    struct sys
    {
        uint32_t free_ddr;
        uint32_t total_ddr;
        uint64_t total_flash;
    } sys;
    char *org;
    char path[MAX_PATH_LEN];
};

#define MAX_TASKS 32

static uint64_t str_to_bytes(const char *str)
{
    uint64_t size;
    char c;

    c = str[strlen(str) - 1];
    size = atoll(str);
    if (size == 0)
    {
        return 0;
    }

    switch (c)
    {
        case '0'...'9':
            return size;
        case 'k':
        case 'K':
            return size * KB;
        case 'm':
        case 'M':
            return size * MB;
        case 'g':
        case 'G':
            return size * GB;
        default:
            return 0;
    }
}

static int randbuf(char *buf, int len)
{
    int x;

    srand((unsigned int)time(NULL));
    while (len)
    {
        int l = min_t(len, (int)sizeof(int));

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

static inline uint64_t get_total_flash(const char *dir)
{
    struct statfs sfs;

    if (statfs(dir, &sfs) < 0)
    {
        printf("statfs %s failed\n", dir);
        return 0;
    }

    return (uint64_t)sfs.f_bsize * (uint64_t)sfs.f_blocks;
}

static inline uint64_t get_free_flash(const char *dir)
{
    struct statfs sfs;

    if (statfs(dir, &sfs) < 0)
    {
        return 0;
    }

    /* get free by f_bavail rather than f_bfree */
    return (uint64_t)sfs.f_bsize * (uint64_t)sfs.f_bfree;
}

static int print_head(struct rwtask *task, int multi_thread)
{
    time_t t = time(NULL);
    struct sys *sys = &task->sys;

    printf("\n\trwcheck: do read and write check\n\n");
    printf("\tversion: %s\n", VERSION);
    printf("\tbuild: %s\n", COMPILE);
    printf("\tdate: %s\n", ctime(&t));
    printf("\tfree/total ddr: %"PRIu64"/%"PRIu64" KB\n",
           sys->free_ddr / KB, sys->total_ddr / KB);
    printf("\tfree/total flash: %"PRIu64"/%"PRIu64" KB\n",
           get_free_flash(task->dir) / KB, sys->total_flash / KB);
    printf("\tset file size to %"PRIu64" KB\n", task->size / KB);
    printf("\tset times to %u\n", task->times);
    printf("\tset max percent of total space to %u%%\n", task->percent);
    printf("\tset check diretory as %s\n", task->dir);
    printf("\tset orgin file as %s\n", task->org);
    printf("\tset multi_thread as %d\n", multi_thread);

    return 0;
}

static int init_orgin(struct rwtask *task)
{
    int fd, size, ret;
    char *buf;

    printf("\t--- INIT ---\n");

    /* if file existed, we should not create new one */
    if (!access(task->org, R_OK))
    {
        return 0;
    }

    fd = open(task->org, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd < 0)
    {
        printf("open %s failed - (%d)\n", task->org, fd);
        return -1;
    }

    printf("\r\tcreate\t: %s ... ", task->org);

    size = task->size;
    buf = malloc(task->g_default_buf_size);
    if (!buf)
    {
        printf("malloc memory failed - (%"PRIu64")bytes\n", task->g_default_buf_size);
        close(fd);
        return -1;
    }
    memset(buf, 0, task->g_default_buf_size);

    while (size)
    {
        int len;

        len = min_t(size, task->g_default_buf_size);
        randbuf(buf, len);
        ret = write(fd, buf, len);
        if (ret != len)
        {
            printf("write %s failed - (%d)\n", task->org, ret);
            ret = -1;
            goto out;
        }

        size -= len;
    }

    printf("\r\tcreate\t: %s ... OK (%"PRIu64"K)\n", task->org, task->size / KB);
    ret = 0;
out:
    if (buf)
    {
        free(buf);
    }
    close(fd);
    return ret;
}

static int deinit_orgin(struct rwtask *task)
{
    remove(task->org);
    return 0;
}

static int copy_file(const char *in, const char *out, uint64_t size, uint64_t g_default_buf_size)
{
    int fd_in, fd_out;
    int ret = -1;
    char *buf = NULL;

    fd_in = open(in, O_RDONLY);
    if (fd_in < 0)
    {
        printf("open %s failed - (%d)\n", in, fd_in);
        goto out;
    }
    fd_out = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_out < 0)
    {
        printf("open %s failed - (%d)\n", out, fd_out);
        goto close_in;
    }

    buf = malloc(g_default_buf_size);
    if (!buf)
    {
        printf("malloc memory failed - (%"PRIu64")bytes\n", g_default_buf_size);
        close(fd_in);
        close(fd_out);
        return -1;
    }
    memset(buf, 0, g_default_buf_size);

    while (size > 0)
    {
        uint64_t sz = min_t(size, (uint64_t)g_default_buf_size);
        uint64_t done = 0;

        while (sz > done)
        {
            int rret;

            rret = read(fd_in, buf + done, sz - done);
            if (rret < 0)
            {
                printf("read %s failed - (%d)\n", in, rret);
                goto close_out;
            }
            /* lseek to start of file to ensure read full size */
            if (rret != sz - done)
            {
                lseek(fd_in, 0, SEEK_SET);
            }
            done += rret;
        }
        done = write(fd_out, buf, sz);
        if (done != sz)
        {
            printf("write %s failed - (%d)\n", out, (int)done);
            goto close_out;
        }
        size -= sz;
    }

    fsync(fd_out);
    ret = 0;
close_out:
    close(fd_out);
close_in:
    close(fd_in);
out:
    if (buf)
    {
        free(buf);
    }
    return ret;
}

static sum_t get_sum_by_buf(const unsigned char *buf, int len)
{
    sum_t sum = 0;

    while (--len >= 0)
    {
        sum += buf[len];
    }

    return sum;
}

static int get_sum_by_file(const char *file, int len, sum_t *sum, uint64_t g_default_buf_size)
{
    int fd, ret = -1;
    unsigned char *buf = NULL;
    struct stat s;

    ret = stat(file, &s);
    if (ret)
    {
        printf("stat %s failed - (%d)\n", file, ret);
        return -1;
    }

    fd = open(file, O_RDWR);
    if (fd < 0)
    {
        printf("open %s failed - (%d)\n", file, fd);
        return -1;
    }

    buf = malloc(g_default_buf_size);
    if (!buf)
    {
        printf("malloc memory failed - (%"PRIu64")bytes\n", g_default_buf_size);
        close(fd);
        return -1;
    }
    memset(buf, 0, g_default_buf_size);

    len = min_t(s.st_size, (off_t)len);
    *sum = 0;
    while (len)
    {
        int rlen = min_t(len, g_default_buf_size);

        ret = read(fd, buf, rlen);
        if (ret != rlen)
        {
            printf("read %s failed - (%d)\n", file, ret);
            close(fd);
            free(buf);
            return -1;
        }

        *sum += get_sum_by_buf(buf, ret);
        len -= ret;
    }

    free(buf);
    close(fd);
    return 0;
}

static int append_sum(const char *file, uint64_t g_default_buf_size)
{
    int fd, ret = -1;
    sum_t sum = 0;
    struct stat s;

    fd = open(file, O_RDWR);
    if (fd < 0)
    {
        printf("open %s failed - (%d)\n", file, fd);
        return -1;
    }

    ret = stat(file, &s);
    if (ret)
    {
        printf("stat %s failed - (%d)\n", file, ret);
        return -1;
    }

    ret = get_sum_by_file(file, s.st_size, &sum, g_default_buf_size);
    if (ret < 0)
    {
        printf("get sum by file %s failed\n", file);
        return -1;
    }

    lseek(fd, 0, SEEK_END);
    ret = write(fd, &sum, sizeof(sum));
    if (ret != sizeof(sum))
    {
        printf("write %s failed - (%d)\n", file, ret);
        goto out;
    }

    ret = 0;
out:
    close(fd);
    return ret;

}

static int read_sum_from_file(const char *file, sum_t *sum)
{
    int fd, ret;

    fd = open(file, O_RDONLY);
    if (fd < 0)
    {
        printf("open %s failed - (%d)\n", file, fd);
        return -1;
    }

    lseek(fd, -sizeof(sum_t), SEEK_END);
    ret = read(fd, sum, sizeof(sum_t));
    if (ret != sizeof(sum_t))
    {
        printf("read sum from %s failed - (%d)\n", file, ret);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static uint64_t get_test_size(struct rwtask *task)
{
    uint64_t free_bytes = get_free_flash(task->dir);

    if (task->size >= free_bytes)
    {
        return 0;
    }

    if (free_bytes - task->size < task->min_free)
    {
        return 0;
    }

    return task->size;
}

static inline const char *get_path(struct rwtask *task, int index)
{
    memset(task->path, 0, MAX_PATH_LEN);
    snprintf(task->path, MAX_PATH_LEN, "%s/%s%d.%d", task->dir,
             DEFAULT_TMP, task->task_it, index);
    return task->path;
}

static int do_create(struct rwtask *task)
{
    uint64_t size = 0;
    const char *path;
    int num = 0, ret = 0;

    printf("\t--- CREATE ---\n");

    while ((size = get_test_size(task)))
    {
        path = get_path(task, num);
        if (!access(path, R_OK))
        {
            return 0;
        }

        printf("\r\tcreate\t: %s ... ", path);

        /* the last 8 bytes should reserved for check sum */
        ret = copy_file(task->org, path, size - sizeof(sum_t), task->g_default_buf_size);
        if (ret)
        {
            return -1;
        }

        /* the last 2 btyes are reserved for saving crc */
        ret = append_sum(path, task->g_default_buf_size);
        if (ret)
        {
            return -1;
        }

        printf("\r\tcreate\t: %s ... OK (%"PRIu64"K)\n", path, size / KB);
        num++;
    }
    return 0;
}

static int do_check(struct rwtask *task)
{
    int num = 0;
    const char *path;

    printf("\t--- CHECK ---\n");

    while (true)
    {
        struct stat s;
        sum_t sum, sum_file;
        int ret = 0;

        path = get_path(task, num);
        if (access(path, R_OK))
        {
            return 0;
        }

        printf("\tcheck\t: %s ... ", path);

        ret = stat(path, &s);
        if (ret)
        {
            printf("stat %s failed - (%d)\n", path, ret);
            return -1;
        }

        if (s.st_size < 8)
        {
            path = get_path(task, num + 1);
            if (!access(path, R_OK))
            {
                printf("FAILED (empty file)\n");
                return -1;
            }
            else
            {
                printf("OK: ignore the last one\n");
                break;
            }
        }

        ret = get_sum_by_file(path, s.st_size - sizeof(sum_t), &sum, task->g_default_buf_size);
        if (ret < 0)
        {
            return -1;
        }

        ret = read_sum_from_file(path, &sum_file);
        if (ret < 0)
        {
            return -1;
        }

        if (sum != sum_file)
        {
            path = get_path(task, num + 1);
            if (!access(path, R_OK))
            {
                printf("FAILED (check sum error 0x%08x != 0x%08x)\n",
                       sum, sum_file);
                return -1;
            }
            else
            {
                printf("OK: ignore the last one\n");
                break;
            }
        }
        else
        {
            printf("OK\n");
        }

        num++;
    }

    return 0;
}

static int get_max_file_index(struct rwtask *task)
{
    int i = 0;
    const char *path;

    while (true)
    {
        path = get_path(task, i);
        if (access(path, R_OK))
        {
            return max_t(0, i - 1);
        }
        i++;
    }
}

static int do_remove(struct rwtask *task)
{
    int num;
    const char *path;

    printf("\t--- REMOVE ---\n");

    num = get_max_file_index(task);

    while (num >= 0)
    {
        path = get_path(task, num);
        if (access(path, R_OK))
        {
            return 0;
        }

        printf("\tremove\t: %s ... ", path);
        if (unlink(path))
        {
            printf("FAILED: %s\n", strerror(errno));
            return -1;
        }
        printf("OK\n");

        num--;
    }
    return 0;
}

static void *rwcheck_thread(void *rwtask_t)
{
    int ret = 0;
    int times = 1;
    struct rwtask *task = (struct rwtask *)rwtask_t;

    while (task->times-- > 0)
    {
        printf("rwcheck times = %d\n", times++);

        ret = init_orgin(task);
        if (ret)
        {
            goto out;
        }

        ret = do_create(task);
        if (ret)
        {
            goto out;
        }

        ret = do_check(task);
        if (ret)
        {
            goto out;
        }

        ret = do_remove(task);
        if (ret)
        {
            goto out;
        }

        ret = deinit_orgin(task);
        if (ret)
        {
            goto out;
        }
    }
out:
    pthread_barrier_wait(task->barrier);
    if (ret)
    {
        printf("task->org(%s) check failed!\n", task->org);
    }
    return NULL;
}

static int begin(struct rwtask *task, int multi_thread)
{
    int i;
    size_t stacksize = 36864;

    print_head(task, multi_thread);

    for (i = 0; i < multi_thread; i++)
    {
        pthread_attr_t attr;
        struct sched_param param;
        char name[32];

        memset(&param, 0, sizeof(struct sched_param));
        memset(&attr, 0, sizeof(pthread_attr_t));
#ifdef CONFIG_SDMMC_CACHE_WRITEBACK_THREAD_PRIO
        param.sched_priority = CONFIG_SDMMC_CACHE_WRITEBACK_THREAD_PRIO;
#else
        param.sched_priority = 22;
#endif
        pthread_attr_init(&attr);
        pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setstacksize(&attr,stacksize);

        pthread_create(&((&task[i])->pth), &attr, rwcheck_thread, &task[i]);

        snprintf((char *)&name, sizeof(name), "%s%d", "rwcheck", i);
        pthread_setname_np((&task[i])->pth, (char *)&name);
    }

    pthread_barrier_wait(task->barrier);

    for (i = 0; i < multi_thread; i++)
    {
        pthread_join((&task[i])->pth, NULL);
    }

    printf("rwcheck OK\n");
    return 0;
}

int rwcheck_main(int argc, char **argv)
{
    int opts = 0;
    int ret = 0;
    int i;
    struct rwtask *task;
    struct rwtask g_task[MAX_TASKS] = {0};

	pthread_barrier_t barrier;

    int g_default_buf_size = (1 * MB);
    int multi_thread = 0;

    for (i = 0; i < sizeof(g_task) / sizeof(g_task[0]); i++)
    {
        memset(&g_task[i], 0, sizeof(g_task[i]));
    }

    task = &g_task[0];
    optind = 0;
    while ((opts = getopt(argc, argv, ":hd:t:s:p:b:j:")) != EOF)
    {
        switch (opts)
        {
            case 'h':
                show_help();
                goto out;
            case 'd':
            {
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
            case 't':
            {
                task->times = atoi(optarg);
                if (!task->times)
                {
                    printf("times %s is zero or invalid\n", optarg);
                    goto out;
                }
                break;
            }
            case 's':
            {
                task->size = str_to_bytes(optarg);
                if (!task->size)
                {
                    printf("size %s is zero or invalid\n", optarg);
                    goto out;
                }
                break;
            }
            case 'p':
            {
                task->percent = atoi(optarg);
                if (!task->percent)
                {
                    printf("percent %s is zero or invalid\n", optarg);
                    goto out;
                }
                break;
            }
            case 'b':
            {
                g_default_buf_size = atoi(optarg);
                if (!g_default_buf_size)
                {
                    printf("g_default_buf_size %s is zero or invalid\n", optarg);
                    goto out;
                }
                break;
            }
            case 'j':
            {
                multi_thread = atoi(optarg);
                if (!multi_thread)
                {
                    printf("multi_thread %s is zero or invalid\n", optarg);
                    goto out;
                }
                if (multi_thread > MAX_TASKS)
                {
                    printf("just support %d tasks!\n", MAX_TASKS);
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

    if (!task->dir)
    {
        printf("Which directory to check? Please tell me by '-d'\n");
        show_help();
        goto out;
    }

    task->g_default_buf_size = g_default_buf_size;

    if (!task->times)
    {
        task->times = DEFAULT_RUN_TIMES;
    }

    if (!task->percent)
    {
        task->percent = DEFAULT_TEST_PERCENT;
    }

    if (!task->size)
    {
        task->size = DEFAULT_TEST_SIZE;
    }

    task->org = malloc(strlen(task->dir) + sizeof(DEFAULT_ORG) + 5);
    if (!task->org)
    {
        printf("malloc for org failed\n");
        goto out;
    }
    sprintf(task->org, "%s/%s", task->dir, DEFAULT_ORG);

    task->sys.free_ddr = get_free_ddr();
    task->sys.total_ddr = get_total_ddr();
    task->sys.total_flash = get_total_flash(task->dir);
    task->min_free = task->sys.total_flash * (100 - task->percent) / 100;
    if (!task->sys.free_ddr || !task->sys.total_ddr ||
        !task->sys.total_flash || !task->min_free)
    {
        printf("invalid sys info: ddr %"PRIu64"/%"PRIu64" KB, flash %"PRIu64" KB, min free %"PRIu64" KB\n",
               task->sys.free_ddr / KB, task->sys.total_ddr / KB,
               task->sys.total_flash / KB, task->min_free / KB);
        goto free_org;
    }

    if (multi_thread < 1)
    {
        multi_thread = 1;
    }

    pthread_barrier_init(&barrier, NULL, multi_thread + 1);
	task->barrier = &barrier;

    for (i = 1; i < multi_thread; i++)
    {
        memcpy(&g_task[i], task, sizeof(g_task[0]));
        g_task[i].org = malloc(strlen(task->dir) + sizeof(DEFAULT_ORG) + 5);
        if (!g_task[i].org)
        {
            printf("malloc for org failed\n");
            goto free_org;
        }
        g_task[i].barrier = &barrier;
        memset(g_task[i].org, 0, strlen(task->dir) + sizeof(DEFAULT_ORG) + 5);
        sprintf(g_task[i].org, "%s%d", task->org, i);
        g_task[i].task_it = i;
    }

    ret = begin(task, multi_thread);

    pthread_barrier_destroy(&barrier);
free_org:
    for (i = 0; i < sizeof(g_task) / sizeof(g_task[0]); i++)
    {
        if (g_task[i].org)
        {
            free(g_task[i].org);
        }
    }
out:
    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(rwcheck_main, __cmd_rwcheck, do read / write check);
