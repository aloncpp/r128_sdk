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

#define CP_BUFSIZE (256)
#define CP_FLAGS_RECURSION (1 << 0)

static void show_help(void)
{
    printf("Usage: cp <from> <to>\n");
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

static int cp_file_to_file(const char *from, const char *to)
{
    int fd_to, fd_from, ret = -1, rlen, wlen;
    char buf[CP_BUFSIZE] = {0};

    fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd_to < 0) {
        printf("open %s failed - %d\n", to, fd_to);
        return -1;
    }

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0) {
        printf("open %s failed - %d\n", from, fd_from);
        goto close_to;
    }

    while ((rlen = read(fd_from, buf, CP_BUFSIZE))) {
        if (rlen < 0) {
            printf("read %s failed - %d\n", from, rlen);
            goto close_from;
        }

        wlen = write(fd_to, buf, rlen);
        if (wlen != rlen) {
            printf("write %s failed - %d\n", to, wlen);
            goto close_from;
        }
    }

    ret = 0;
close_from:
    close(fd_from);
close_to:
    close(fd_to);
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

static int cp_file_to_dir(const char *from, const char *to)
{
    int ret;
    char *_to = get_path(to, base_name(from));

    if (!_to) {
        return -1;
    }
    ret = cp_file_to_file(from, _to);

    free(_to);
    return ret;
}

static int cp_dir_to_dir(const char *from, const char *to)
{
    DIR *pdir = NULL;
    struct dirent *entry = NULL;
    int ret = -1;
    char *to_dir;

    to_dir = get_path(to, base_name(from));
    if (!to_dir) {
        return -1;
    }

    ret = mkdir(to_dir, 0777);
    if (ret) {
        printf("mkdir %s failed - %d\n", to_dir, ret);
        goto free;
    }

    pdir = opendir(from);
    if (!pdir) {
        printf("opendir %s failed\n", from);
        goto free;
    }

    while ((entry = readdir(pdir))) {
        char *fpath = get_path(from, entry->d_name);

        ret = cp_file_to_dir(fpath, to_dir);
        free(fpath);
        if (ret)
            goto out;
    }

    ret = 0;
out:
    closedir(pdir);
free:
    free(to_dir);
    return ret;
}

static int cp_do(char *from, char *to, int flags)
{
    int ret = -1, fromdir = false, todir = false;

    fromdir = isdir(from);
    if (fromdir < 0) {
        printf("%s do not existed\n", from);
        return -1;
    } else if ((bool)fromdir == true && !(flags & CP_FLAGS_RECURSION)) {
        printf("%s is a directory\n", from);
        return -1;
    }

    todir = isdir(to);
    /* do not existed */
    if (todir < 0)
        todir = false;

    if (!fromdir && !todir)
        ret = cp_file_to_file(from, to);
    else if (!fromdir && todir)
        ret = cp_file_to_dir(from, to);
    else if (fromdir && todir)
        ret = cp_dir_to_dir(from, to);
    else
        printf("not support copy directory to file\n");

    return ret;
}

static int cp_main(int argc, char **argv)
{
    int opts = 0, flags = 0;

    optind = 0;
    while ((opts = getopt(argc, argv, ":rh")) != EOF) {
        switch (opts) {
        case 'r': flags |= CP_FLAGS_RECURSION; break;
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

    if (argc != 2) {
        show_help();
        return -1;
    }

    return cp_do(argv[0], argv[1], flags);
}
FINSH_FUNCTION_EXPORT_CMD(cp_main, cp, copy file);
