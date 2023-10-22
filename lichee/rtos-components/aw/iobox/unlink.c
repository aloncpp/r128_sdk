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

static void show_help(void)
{
    printf("Usage: unlink <file1> [<file2>...]\n");
}

static int unlink_main(int argc, char **argv)
{
    int index, ret;

    if (argc < 2) {
        show_help();
        return -1;
    }

    for (index = 1; index < argc; index++) {
        struct stat s;
        const char *file = argv[index];

        ret = stat(file, &s);
        if (ret < 0) {
            printf("stat %s failed - %s\n", file, strerror(errno));
            return -1;
        }
        if (S_ISDIR(s.st_mode)) {
            printf("%s is a directory\n", file);
            return -1;
        }

        ret = unlink(file);
        if (ret) {
            printf("unlink %s failed - %s\n", file, strerror(errno));
            return -1;
        }
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(unlink_main, unlink, unlink file);
