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
#include <stdlib.h>

#define MKDIR_FLAGS_PARENTS (1 << 0)
static void show_help(void)
{
    printf("Usage: mkdir [-p] <dir>\n");
    printf("  -p: make the upper directory if needed.\n");
}

static int mkdir_do(char *path, int flags)
{
    char *s = NULL;

    if (flags & MKDIR_FLAGS_PARENTS) {
        s = path;
    }
    /*
     * All of operations must base on root directory
     * As alios has not root dierctory, we can operate '/data' but not '/'
     */
    if (path[0] == '.') {
        if (path[1] == '\0')
            return 0;
        if (path[1] == '.' && path[2] == '\0')
            return 0;
    }

    while (1) {
        struct stat st;

        if (flags & MKDIR_FLAGS_PARENTS) {
            /* in case of tailing '/', such as '/data/a/' */
            if (*(s++) == '\0')
                break;
            s = strchr(s, '/');
            if (s)
                *s = '\0';
        }

        if (!stat(path, &st)) {
            if (S_ISDIR(st.st_mode))
                goto next;
            printf("make failed - %s already existed and not direcotry\n", path);
            return -1;
        }

        if (mkdir(path, 0777) < 0) {
            printf("mkdir %s failed\n", path);
            return -1;
        }

next:
        if (!s)
            break;
        *s = '/';
    }
    return 0;
}

static int mkdir_main(int argc, char **argv)
{
    int opts = 0, ret = 0, flags = 0, index;

    optind = 0;
    while ((opts = getopt(argc, argv, ":ph")) != EOF) {
        switch (opts) {
        case 'p': flags |= MKDIR_FLAGS_PARENTS; break;
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

    if (argc < 1) {
        show_help();
        return -1;
    }

    for (index = 0; index < argc; index++) {
        char *path;

        path = strdup(argv[index]);
        ret = mkdir_do(path, flags);
        free(path);
        if (ret)
            return ret;
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(mkdir_main, mkdir, make directory);
