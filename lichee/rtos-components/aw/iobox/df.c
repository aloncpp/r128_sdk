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
#include <sys/statfs.h>
#include <sys/types.h>
#include <dirent.h>

#define KB (1024ULL)
static void df_do_dir(const char *dir)
{
    struct statfs sfs;
    unsigned long long total, used, free;

    if (statfs(dir, &sfs) < 0) {
        printf("statfs %s failed\n", dir);
        return;
    }

    total = sfs.f_bsize * sfs.f_blocks / KB;
    free = sfs.f_bsize * sfs.f_bavail / KB;
    used = total - free;
    printf("%7llu%7llu%7llu%6llu%% %s\n", total, used, free,
            used * 100 / total, dir);
}

static int df_root(void)
{
    DIR *pdir;
    struct dirent *entry = NULL;

    pdir = opendir("/");
    if (!pdir)
        return -1;

    while ((entry = readdir(pdir))) {
        char fpath[128] = {0};

        snprintf(fpath, 128, "/%s", entry->d_name);
        df_do_dir(fpath);
    }

    closedir(pdir);
}

static int df_main(int argc, char **argv)
{
    int i;

    printf("Unit(KB)\r\n");
    printf("%7s%7s%7s%7s %s\n", "Total", "Used", "Free", "Use%", "Mount");

    if (argc <= 1) {
        return df_root();
    }

    for (i = 1; i < argc; i++)
        df_do_dir(argv[i]);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(df_main, df, copy file);
