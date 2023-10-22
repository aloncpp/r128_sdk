#include <stdio.h>
#include <tinatest.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <console.h>
#include <stdbool.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>

static void show_help(void)
{
    printf("Usage: mem_to_file <addr> <to> <size>\n");
}

static void show_file_to_mem_help(void)
{
    printf("Usage: file_to_mem <addr> <from> <size> <skipsize>\n");
}

static void show_mem_to_nor_help(void)
{
    printf("Usage: mem_to_nor <addr> <nor offset> <size>\n");
}

/* less than 0 means not existed */
static int isdir(const char *path)
{
    struct stat s;
    int ret;

    ret = stat(path, &s);
    if (ret)
        return -1;
    return S_ISDIR(s.st_mode) ? true : false;
}

static int filesize(const char *path)
{
    struct stat s;
    int ret;

    ret = stat(path, &s);
    if (ret)
        return -1;
    return s.st_size;
}

static int mem_to_file_do(const char *buf, const char *to, int size)
{
    int fd_to, ret = -1, wlen;

    fd_to = open(to, O_WRONLY | O_CREAT | O_APPEND);
    if (fd_to < 0) {
        printf("open %s failed - %d\n", to, fd_to);
        return -1;
    }

    wlen = write(fd_to, buf, size);
    if (wlen != size) {
        printf("write %s failed - %d\n", to, wlen);
        ret = -1;
        goto close_to;
    }

    ret = 0;
close_to:
    close(fd_to);
    return ret;
}

static int file_to_mem_do(const char *buf, const char *from, int size, int skip_size)
{
    int fd_from, ret = -1, rlen, fsize;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0) {
        printf("open %s failed - %d\n", from, fd_from);
        return -1;
    }

    fsize = filesize(from);
    printf("file size: %d = 0x%x\n", fsize, fsize);
    if (size > fsize - skip_size) {
	printf("file size - seek size: %d - %d < %d, only read file size\n", fsize, skip_size, size);
	size = fsize - skip_size;
    }

    ret = lseek(fd_from, skip_size, SEEK_SET);
    if (ret < 0) {
        printf("lseek %d in %s failed - %s\n", skip_size, from, strerror(errno));
        goto close_from;
    }

    rlen = read(fd_from, (void *)buf, size);
    if (rlen != size) {
        printf("read %s failed - %d\n", from, rlen);
        ret = -1;
        goto close_from;
    }

    ret = 0;
close_from:
    close(fd_from);
    return ret;
}

static char *get_path(const char *dir, const char *file)
{
    int ret;
    char *path;

    path = malloc(128);
    if (!path)
        return NULL;

    if (file[0] == '/')
        ret = sprintf(path, "%s%s", dir, file);
    else
        ret = sprintf(path, "%s/%s", dir, file);

    if (ret <= 0) {
        free(path);
        path = NULL;
    }
    return path;
}

static const char *base_name(const char *path)
{
    char *p = strrchr(path, '/');

    return p ? p + 1 : path;
}

static int mem_to_dir_do(const char *from, const char *to, int size)
{
    int ret;
    char *_to = get_path(to, "/");

    ret = mem_to_file_do(from, _to, size);

    free(_to);
    return ret;
}

static int do_mem_to_file(char *from, char *to, int size)
{
    int ret = -1, fromdir = false, todir = false;


    todir = isdir(to);
    /* do not existed */
    if (todir < 0)
        todir = false;

    if (!todir)
        ret = mem_to_file_do(from, to, size);
    else if (todir)
        ret = mem_to_dir_do(from, to, size);

    return ret;
}


static int do_file_to_mem(char *to, char *from, int size, int skip_size)
{
    int ret = -1;

    ret = file_to_mem_do(to, from, size, skip_size);

    return ret;
}

static int mem_to_file_main(int argc, char **argv)
{
    int opts = 0, flags = 0, size = 0;

    optind = 0;
    while ((opts = getopt(argc, argv, ":h")) != EOF) {
        switch (opts) {
        case 'h': show_help(); return 0;
        case '?': printf("invalid option %c\n", optopt); return -1;
        case ':':
            printf("option -%c requires an argument\n", optopt);
            show_help();
            return -1;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 3) {
        show_help();
        return -1;
    }

    return do_mem_to_file((char *)(unsigned long)atoi(argv[0]), argv[1], atoi(argv[2]));
}
FINSH_FUNCTION_EXPORT_CMD(mem_to_file_main, mem_to_file, append memory data to file);

static int file_to_mem_main(int argc, char **argv)
{
    int opts = 0, flags = 0, size = 0;

    optind = 0;
    while ((opts = getopt(argc, argv, ":h")) != EOF) {
        switch (opts) {
        case 'h': show_file_to_mem_help(); return 0;
        case '?': printf("invalid option %c\n", optopt); return -1;
        case ':':
            printf("option -%c requires an argument\n", optopt);
            show_file_to_mem_help();
            return -1;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 4) {
        show_file_to_mem_help();
        return -1;
    }

    return do_file_to_mem((char *)(unsigned long)atoi(argv[0]), argv[1], atoi(argv[2]), atoi(argv[3]));
}
FINSH_FUNCTION_EXPORT_CMD(file_to_mem_main, file_to_mem, read file to memory);

static int do_mem_to_nor_main(char *buf, int offset, int size)
{
    int ret = -1, fd_to = -1;

    fd_to = open("/dev/nor0", O_WRONLY);
    if (fd_to < 0)
        return -1;

    ret = lseek(fd_to, offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek /dev/nor0 failed - %s\n", strerror(errno));
        goto close_nor;
    }
    ret = write(fd_to, buf, size);
    if (ret != size) {
        printf("write failed - %d\n", ret);
        goto close_nor;
    }

    ret = 0;
close_nor:
    close(fd_to);
    return ret;
}

static int mem_to_nor_main(int argc, char **argv)
{
    int opts = 0, flags = 0, size = 0;

    optind = 0;
    while ((opts = getopt(argc, argv, ":h")) != EOF) {
        switch (opts) {
        case 'h': show_mem_to_nor_help(); return 0;
        case '?': printf("invalid option %c\n", optopt); return -1;
        case ':':
            printf("option -%c requires an argument\n", optopt);
            show_mem_to_nor_help();
            return -1;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 3) {
        show_mem_to_nor_help();
        return -1;
    }

    return do_mem_to_nor_main((char *)(unsigned long)atoi(argv[0]), atoi(argv[1]), atoi(argv[2]));
}
FINSH_FUNCTION_EXPORT_CMD(mem_to_nor_main, mem_to_nor, write memory data to nor);

static int malloc_main(int argc, char **argv)
{
    int size = atoi(argv[1]);

    char* addr = malloc(size);
    if (!addr) {
        printf("malloc %d fail\n", size);
        return -1;
    }
    printf("malloc %d in 0x%lx = %ld\n", size, (unsigned long)addr, (unsigned long)addr);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(malloc_main, mem_malloc, malloc space);

static int free_main(int argc, char **argv)
{
    char* addr = (void *)atoi(argv[1]);
    printf("free 0x%lx = %ld\n", (unsigned long)addr, (unsigned long)addr);

    free(addr);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(free_main, mem_free, free space);
