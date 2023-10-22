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

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#define CAT_BUFSIZE (256)

static int cat_main(int argc, char **argv)
{
    int index;

    for (index = 1; index < argc; index++) {
        char *file = argv[index];
        struct stat s;
        int ret, fd;
        __attribute__((aligned(64))) char buf[CAT_BUFSIZE];

        ret = stat(file, &s);
        if (ret) {
            printf("stat %s failed - %s\n", file, strerror(errno));
            continue;
        }

        fd = open(file, O_RDONLY);
        if (fd < 0) {
            printf("open %s failed - %s\n", file, strerror(errno));
            continue;
        }

        while ((ret = read(fd, buf, CAT_BUFSIZE))) {
            int wlen;

            if (ret < 0) {
                printf("read %s failed - %s\n", file, strerror(errno));
                goto close;
            }

            wlen = write(STDOUT_FILENO, buf, ret);
            if (wlen != ret) {
                printf("write %s failed - %s\n", file, strerror(errno));
                goto close;
            }
        }
close:
        close(fd);
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cat_main, cat, read file);
