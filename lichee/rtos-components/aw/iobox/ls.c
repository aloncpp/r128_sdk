#include <stdio.h>
#include <tinatest.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <console.h>
#include <stdbool.h>

#define LONGLIST    (1 << 0)
#define SZ_KB       (1 << 1)
#define NEW_LINE    (1 << 2)

static void show_help(void)
{
    printf("Usage: ls [-l] [-h] [-k] <file>...\n");
    printf("  -l: use long list\n");
    printf("  -h: show this help message and exit");
    printf("  -k: use 1024-byte blocks");
}

static const char *relative_path(const char *path, const char *dir)
{
    const char *p = path + strlen(dir);

    while (*p == '/')
        p++;
    return p < path + strlen(path) ? p : NULL;
}

static void ls_show(const char *file, int flags, struct stat s)
{
    if (!(flags & LONGLIST)) {
        if (flags & NEW_LINE)
            printf("%s\n", file);
        else
            printf("%s ", file);
        return;
    }

    switch (s.st_mode & S_IFMT) {
    case S_IFBLK: printf("%c", 'b'); break;
    case S_IFCHR: printf("%c", 'c'); break;
    case S_IFDIR: printf("%c", 'd'); break;
    case S_IFLNK: printf("%c", 'l'); break;
    case S_IFREG: printf("%c", '-'); break;
    default: printf("%c", '?'); break;
    }
    printf("%c%c%c%c%c%c%c%c%c root root %lu%sB %s\n",
            s.st_mode & S_IRUSR ? 'r' : '-',
            s.st_mode & S_IWUSR ? 'w' : '-',
            s.st_mode & S_IXUSR ? 'x' : '-',
            s.st_mode & S_IRGRP ? 'r' : '-',
            s.st_mode & S_IWGRP ? 'w' : '-',
            s.st_mode & S_IXGRP ? 'x' : '-',
            s.st_mode & S_IROTH ? 'r' : '-',
            s.st_mode & S_IWOTH ? 'w' : '-',
            s.st_mode & S_IXOTH ? 'x' : '-',
            flags & SZ_KB ? s.st_size / 1024 : s.st_size,
            flags & SZ_KB ? "K" : "", file);
}

static int ls_do(int argc, char **argv, int flags)
{
    int index;
    char *root[1] = {"/"};

    /* default root */
    if (argc == 0) {
        argc = 1;
#ifdef CONFIG_USING_WORKDIR
        extern char working_directory[];
        argv[0] = (char *)&working_directory;
#else
        argv[0] = root[0];
#endif
    }

    for (index = 0; index < argc; index++) {
        char *dir = argv[index];
        DIR *pdir = NULL;
        struct dirent *entry = NULL;
        struct stat s;
        int cnt = 1;

        if (stat(dir, &s)) {
            printf("%s not existed\n", dir);
            continue;
        }
		if (!S_ISDIR(s.st_mode)) {
            flags |= NEW_LINE;
            ls_show(dir, flags, s);
            if (argc > 1 && index + 1 < argc)
                printf("\n");
            continue;
        } else {
            if (argc > 1)
                printf("%s:\n", dir);

            pdir = opendir(dir);
            if (!pdir) {
                printf("opendir %s failed - %s\n", argv[index], strerror(errno));
                continue;
            }

            while ((entry = readdir(pdir))) {
                char fpath[128];
                int len;

                memset(fpath, 0, 128);
                len = strlen(dir);
#if AOS_COMP_SPIFFS
                /*
                 * fix for spiffs
                 * readdir/opendir on spiffs will traverse all
                 * files in all directories
                 */
                snprintf(fpath, 128, "/data/%s", entry->d_name);
                if (strncmp(dir, fpath, len))
                    continue;
#else
                if (dir[len] == '/' || len == 1)
                    snprintf(fpath, 128, "%s%s", dir, entry->d_name);
                else
                    snprintf(fpath, 128, "%s/%s", dir, entry->d_name);
#endif
                if (stat(fpath, &s)) {
                    printf("stat %s failed - %s\n", fpath, strerror(errno));
                    continue;
                }

                if (!(cnt % 5))
                    flags |= NEW_LINE;
                else
                    flags &= ~NEW_LINE;
                cnt++;

                ls_show(relative_path(fpath, dir), flags, s);
            }
            closedir(pdir);
            if (!(flags & LONGLIST) && !(flags & NEW_LINE))
                printf("\n");
            if (argc > 1 && index + 1 < argc)
                printf("\n");
        }
    }
    return 0;
}

static int ls_main(int argc, char **argv)
{
    int opts = 0, flags = 0;

    optind = 0;
    while ((opts = getopt(argc, argv, ":lhk")) != EOF) {
        switch (opts) {
        case 'l': flags |= LONGLIST; break;
        case 'k': flags |= SZ_KB; break;
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

    return ls_do(argc, argv, flags);
}
FINSH_FUNCTION_EXPORT_CMD(ls_main, ls, list file or directory);

static int ll_main(int argc, char **argv)
{
    argc -= 1;
    argv += 1;

    return ls_do(argc, argv, LONGLIST | SZ_KB);
}
FINSH_FUNCTION_EXPORT_CMD(ll_main, ll, the same as 'ls -kl');
