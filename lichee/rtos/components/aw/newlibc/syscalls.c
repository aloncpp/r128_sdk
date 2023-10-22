#include <reent.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/times.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <spinlock.h>
#include <hal_uart.h>

#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>

#ifdef CONFIG_COMPONENTS_MULTI_CONSOLE
#include <cli_console.h>
#endif

#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"
#include "FreeRTOS_POSIX/utils.h"

#include <ktimer.h>

#ifdef CONFIG_COMPONENT_VFS
#include <vfs.h>
#endif

#ifdef CONFIG_COMPONENTS_VIRT_LOG
#include <virt_log.h>
#endif
/* Reentrant versions of system calls.  */

#ifndef MILLISECOND_PER_SECOND
#define MILLISECOND_PER_SECOND  1000UL
#endif

#ifndef MICROSECOND_PER_SECOND
#define MICROSECOND_PER_SECOND  1000000UL
#endif

#ifndef NANOSECOND_PER_SECOND
#define NANOSECOND_PER_SECOND   1000000000UL
#endif

#define MILLISECOND_PER_TICK    (MILLISECOND_PER_SECOND / RT_TICK_PER_SECOND)
#define MICROSECOND_PER_TICK    (MICROSECOND_PER_SECOND / RT_TICK_PER_SECOND)
#define NANOSECOND_PER_TICK     (NANOSECOND_PER_SECOND  / RT_TICK_PER_SECOND)

pthread_mutex_t timeofday_mutex = FREERTOS_POSIX_MUTEX_INITIALIZER;
static struct timeval sys_time = {0};
static uint64_t change_time_count = 0;

static uint64_t tick_count = 0;

#ifdef CONFIG_CHECK_ILLEGAL_FUNCTION_USAGE
static void __attribute__((no_instrument_function)) cyg_profile_print_hex(int h)
{
    int x;
    int c;
    for (x = 0; x < 8; x++)
    {
        c = (h >> 28) & 0xf;
        if (c < 10)
        {
            hal_uart_put_char(CONFIG_CLI_UART_PORT, '0' + c);
        }
        else
        {
            hal_uart_put_char(CONFIG_CLI_UART_PORT, 'a' + c - 10);
        }
        h <<= 4;
    }
}

static int cyg_profile_func_init = 0;

void __attribute__((no_instrument_function)) __cyg_profile_func_init(void)
{
    cyg_profile_func_init = 1;
}

void __attribute__((no_instrument_function))
__cyg_profile_func_enter(void *this_func, void *call_site)
{
extern char __XIP_Base[];
#define FLASH_XIP_START_ADDR                    ((uintptr_t)(__XIP_Base))
    /* check the this_func is stored in flash(xip) and called by ISR  */
    if ((cyg_profile_func_init != 0) && ((unsigned long)this_func >= FLASH_XIP_START_ADDR) && (hal_interrupt_get_nest() != 0)) {
        hal_uart_put_char(CONFIG_CLI_UART_PORT, 'f');
        hal_uart_put_char(CONFIG_CLI_UART_PORT, 'u');
        hal_uart_put_char(CONFIG_CLI_UART_PORT, 'n');
        hal_uart_put_char(CONFIG_CLI_UART_PORT, 'c');
        hal_uart_put_char(CONFIG_CLI_UART_PORT, ':');
        cyg_profile_print_hex((int)(unsigned long)this_func);
        hal_uart_put_char(CONFIG_CLI_UART_PORT, '\r');
        hal_uart_put_char(CONFIG_CLI_UART_PORT, '\n');

        hal_uart_put_char(CONFIG_CLI_UART_PORT, 'c');
        hal_uart_put_char(CONFIG_CLI_UART_PORT, 'a');
        hal_uart_put_char(CONFIG_CLI_UART_PORT, 'l');
        hal_uart_put_char(CONFIG_CLI_UART_PORT, 'l');
        hal_uart_put_char(CONFIG_CLI_UART_PORT, ':');
        cyg_profile_print_hex((int)(unsigned long)call_site);
        hal_uart_put_char(CONFIG_CLI_UART_PORT, '\r');
        hal_uart_put_char(CONFIG_CLI_UART_PORT, '\n');
    }
}

void __attribute__((no_instrument_function))
__cyg_profile_func_exit(void *this_func, void *call_site)
{
}
#endif

int _close_r(struct _reent *ptr, int fd)
{
    if (fd < 3) {
        ptr->_errno = EINVAL;
        return -1;
    }
#ifdef CONFIG_COMPONENT_VFS
    return vfs_close(ptr, fd);
#elif defined(CONFIG_AMP_FSYS_STUB)
    return close(fd);
#else
    ptr->_errno = ENOSYS;
    return -1;
#endif
}

int _execve_r(struct _reent *ptr, const char *name, char *const *argv, char *const *env)
{
    /* return "not supported" */
    ptr->_errno = ENOTSUP;
    return -1;
}

int _fcntl_r(struct _reent *ptr, int fd, int cmd, int arg)
{
    /* return "not supported" */
    ptr->_errno = ENOTSUP;
    return -1;
}

int _fork_r(struct _reent *ptr)
{
    /* return "not supported" */
    ptr->_errno = ENOTSUP;
    return -1;
}

int _fstat_r(struct _reent *ptr, int fd, struct stat *pstat)
{
    if (fd < 3) {
        ptr->_errno = ENOSYS;
        return -1;
    }
#ifdef CONFIG_COMPONENT_VFS
    return vfs_fstat(ptr, fd, pstat);
#elif defined(CONFIG_AMP_FSYS_STUB)
    return fstat(fd, pstat);
#else
    ptr->_errno = ENOSYS;
    return -1;
#endif
}

int _getpid_r(struct _reent *ptr)
{
    TaskHandle_t current_task = NULL;
    current_task = xTaskGetCurrentTaskHandle();
    if(current_task)
    {
        return uxTaskGetTaskNumber(current_task);
    }
    else
    {
        return 0;
    }
}

int getuid(void)
{
    return getpid();
}

mode_t umask(mode_t mask)
{
    return mask;
}

int _isatty_r(struct _reent *ptr, int fd)
{
    if (fd >= 0 && fd < 3)
    {
        return 1;
    }

    /* return "not supported" */
    ptr->_errno = ENOTSUP;
    return 0;
}

int _isatty(int fd)
{
    if (fd >= 0 && fd < 3)
    {
        return 1;
    }

    return 0;
}

int _kill_r(struct _reent *ptr, int pid, int sig)
{
    /* return "not supported" */
    ptr->_errno = ENOTSUP;
    return -1;
}

int _link_r(struct _reent *ptr, const char *old, const char *new)
{
#ifdef CONFIG_COMPONENT_VFS
    return vfs_link(ptr, old, new);
#elif defined(CONFIG_AMP_FSYS_STUB)
    return link(old, new);
#else
    ptr->_errno = ENOSYS;
    return -1;
#endif
}

_off_t _lseek_r(struct _reent *ptr, int fd, _off_t pos, int whence)
{
    if (fd < 3) {
        ptr->_errno = ENOSYS;
        return -1;
    }
#ifdef CONFIG_COMPONENT_VFS
    return vfs_lseek(ptr, fd, pos, whence);
#elif defined(CONFIG_AMP_FSYS_STUB)
    return lseek(fd, pos, whence);
#else
    ptr->_errno = ENOSYS;
    return -1;
#endif
}

int _mkdir_r(struct _reent *ptr, const char *name, int mode)
{
    ptr->_errno = ENOTSUP;
    return -1;
}

int _open_r(struct _reent *ptr, const char *file, int flags, int mode)
{
#ifdef CONFIG_COMPONENT_VFS
    return vfs_open(ptr, file, flags, mode);
#elif defined(CONFIG_AMP_FSYS_STUB)
    return open(file, flags, mode);
#else
    ptr->_errno = ENOSYS;
    return -1;
#endif
}

_ssize_t _read_r(struct _reent *ptr, int fd, void *buf, size_t nbytes)
{
#ifdef CONFIG_DRIVERS_UART
    int port = CONFIG_CLI_UART_PORT;
    if (fd < 3)
    {
#ifdef CONFIG_COMPONENTS_MULTI_CONSOLE
        /* param console is not NULL, to avoid read data from other console when using redirect command.  */
        nbytes = cli_console_read(get_clitask_console(), buf, nbytes);
#else
#if defined(CONFIG_COMPONENT_CLI) || defined(CONFIG_COMPONENT_FINSH_CLI)
        nbytes = hal_uart_receive(port, buf, nbytes);
#else
        while(1) {
            sleep(60 * 60 * 24 * 30);
        }
#endif
#endif
        return (nbytes);
    }
#endif
#ifdef CONFIG_COMPONENT_VFS
    return vfs_read(ptr, fd, buf, nbytes);
#elif defined(CONFIG_AMP_FSYS_STUB)
    return read(fd, buf, nbytes);
#else
    ptr->_errno = ENOSYS;
    return -1;
#endif
}

int _rename_r(struct _reent *ptr, const char *old, const char *new)
{
#ifdef CONFIG_COMPONENT_VFS
    return vfs_rename(ptr, old, new);
#elif defined(CONFIG_AMP_FSYS_STUB)
    return rename(old, new);
#else
    ptr->_errno = ENOSYS;
    return -1;
#endif
}

extern uint8_t __bss_end[];

void *_sbrk_r(struct _reent *ptr, ptrdiff_t incr)
{
//#define HEAP_TOP    (CONFIG_DRAM_VIRTBASE + CONFIG_DRAM_SIZE)
#define HEAP_TOP    0x1000000
    static void *heap_end;
    void *prev_heap_end;

    if (heap_end == NULL)
    {
        heap_end = (void *)&__bss_end[0];
    }

    prev_heap_end = heap_end;

    if (heap_end  + incr > (void *)HEAP_TOP)
    {
        return NULL;
    }

    heap_end += incr;
    /* no use this routine to get memory */
    return prev_heap_end;
}

int _stat_r(struct _reent *ptr, const char *file, struct stat *pstat)
{
#ifdef CONFIG_COMPONENT_VFS
    return vfs_stat(ptr, file, pstat);
#elif defined(CONFIG_AMP_FSYS_STUB)
    return stat(file, pstat);
#else
    ptr->_errno = ENOSYS;
    return -1;
#endif
}

_CLOCK_T_ _times_r(struct _reent *r, struct tms *ptms)
{
    struct timeval tv = {0, 0};
    _gettimeofday_r(r, &tv, NULL);

    if (ptms) {
        ptms->tms_cstime = 0;
        ptms->tms_cutime = 0;
        ptms->tms_stime = (clock_t)(tv.tv_sec);
        ptms->tms_utime = 0;
    }
    return (clock_t) tv.tv_sec;
}


int _unlink_r(struct _reent *ptr, const char *file)
{
#ifdef CONFIG_COMPONENT_VFS
    return vfs_unlink(ptr, file);
#elif defined(CONFIG_AMP_FSYS_STUB)
    return unlink(file);
#else
    ptr->_errno = ENOSYS;
    return -1;
#endif
}

int _wait_r(struct _reent *ptr, int *status)
{
    /* return "not supported" */
    ptr->_errno = ENOTSUP;
    return -1;
}

_ssize_t _write_r(struct _reent *ptr, int fd, const void *buf, size_t nbytes)
{
    if (fd < 3) {
#ifdef CONFIG_COMPONENTS_VIRT_LOG
        if (virt_log_is_enable()) {
            virt_log_put_buf_len((char *)buf, nbytes);
            return nbytes;
        }
#endif

#ifdef CONFIG_COMPONENTS_MULTI_CONSOLE
        cli_console_write(NULL, buf, nbytes);
#else
#if defined(CONFIG_DRIVERS_UART) && !defined(CONFIG_DISABLE_ALL_UART_LOG)
        char *b = (char *)buf;
        int i;
        for (i = 0; i < nbytes; i ++)
        {
            if (*(b + i) == '\n')
            {
                hal_uart_send(CONFIG_CLI_UART_PORT, "\r", 1);
            }
            hal_uart_send(CONFIG_CLI_UART_PORT, b + i, 1);
        }
#endif
#endif
        return (nbytes);
    }

#ifdef CONFIG_COMPONENT_VFS
    return vfs_write(ptr, fd, buf, nbytes);
#elif defined(CONFIG_AMP_FSYS_STUB)
    return write(fd, buf, nbytes);
#else
    ptr->_errno = ENOSYS;
    return -1;
#endif
}

int gettimeofday_nolock(struct timeval *tv)
{
    int fd = -1;
    int ret = -1;
    int64_t nsecs = ktime_get();
    int64_t dnsecs = (nsecs > change_time_count) ? (nsecs - change_time_count) : 0LL;

    change_time_count = nsecs;

    struct timeval tmp;
    tmp.tv_sec  = (dnsecs / NSEC_PER_USEC) / USEC_PER_SEC;
    tmp.tv_usec = (dnsecs - (tmp.tv_sec * NSEC_PER_USEC * USEC_PER_SEC)) / NSEC_PER_USEC;

    if (ret)
    {
        sys_time.tv_sec += tmp.tv_sec;
        sys_time.tv_usec += tmp.tv_usec;

        if(sys_time.tv_usec >= (USEC_PER_SEC)) {
            sys_time.tv_sec ++;
            sys_time.tv_usec -= (USEC_PER_SEC);
        }

    }

    if (tv != NULL)
    {
        tv->tv_sec = sys_time.tv_sec;
        tv->tv_usec = sys_time.tv_usec;
    }

    return 0;
}

int _gettimeofday_r(struct _reent *ptr, struct timeval *__tp, void *__tzp)
{
    int64_t nsecs  = 0;
    int64_t dnsecs = 0;
    struct timeval tmp = {0};

   pthread_mutex_lock(&timeofday_mutex);

    nsecs = ktime_get();

    if(nsecs >= change_time_count)
        dnsecs = nsecs - change_time_count;
    else
        dnsecs = 0;

    change_time_count = nsecs;

    tmp.tv_sec  = (dnsecs / NSEC_PER_USEC) / USEC_PER_SEC;
    tmp.tv_usec = (dnsecs - (tmp.tv_sec * NSEC_PER_USEC * USEC_PER_SEC)) / NSEC_PER_USEC;

    sys_time.tv_sec += tmp.tv_sec;
    sys_time.tv_usec += tmp.tv_usec;

    if(sys_time.tv_usec >= (USEC_PER_SEC)) {
        sys_time.tv_sec ++;
        sys_time.tv_usec -= ( USEC_PER_SEC );
    }

    if (__tp != NULL)
    {
        __tp->tv_sec = sys_time.tv_sec;
        __tp->tv_usec = sys_time.tv_usec;
    }

    pthread_mutex_unlock(&timeofday_mutex);

    return 0;
}

void __env_lock(struct _reent *_r)
{

}

void __env_unlock(struct _reent *_r)
{

}


void __malloc_lock(struct _reent *_r)
{

}

void __malloc_unlock(struct _reent *_r)
{

}

/* Memory routine */
void *_malloc_r(struct _reent *ptr, size_t size)
{
    void *result = NULL;

    result = pvPortMalloc(size);
    if (result == NULL)
    {
        ptr->_errno = ENOMEM;
    }

    return result;
}

void *_realloc_r(struct _reent *ptr, void *old, size_t newlen)
{
    void *pvPortRealloc(void * old, size_t newlen);
    return pvPortRealloc(old, newlen);
}

void *_calloc_r(struct _reent *ptr, size_t size, size_t len)
{
    void *pvPortCalloc(size_t count, size_t size);
    return pvPortCalloc(size, len);
}

void _free_r(struct _reent *ptr, void *addr)
{
    vPortFree(addr);
}

void _exit(int status)
{
    printf("error status : %d\n", status);
    while (1);
}
#if 0
int _system_r(struct _reent *r, const char *str)
{
    return -1;
}
#endif

void _system(const char *s)
{
    /* not support this call */
    return;
}

void __libc_init_array(void)
{
    /* we not use __libc init_aray to initialize C++ objects */
}

int settimeofday (const struct timeval * tm_val, const struct timezone * tz)
{
    pthread_mutex_lock(&timeofday_mutex);

    change_time_count = ktime_get();
    sys_time.tv_sec = tm_val->tv_sec;
    sys_time.tv_usec = tm_val->tv_usec;

    pthread_mutex_unlock(&timeofday_mutex);
    return 0;
}
