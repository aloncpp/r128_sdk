#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <awlog.h>
#include <tinatest.h>
#include <sunxi_hal_spinor.h>
#include <stdlib.h>
#include <spiffs.h>
#include <devfs.h>

#define TEST_BUF_SIZE 256

static bool init_end = false;
static struct {
    int fd;
    char path[32];
} tt_fds[16] = {0};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

static int vfs_show_help(void);

static int path_to_fd(const char *path)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(tt_fds); i++) {
        if (tt_fds[i].fd < 3)
            continue;
        if (!strcmp(tt_fds[i].path, path))
            return tt_fds[i].fd;
    }
    return -1;
}

static const char *fd_to_path(int fd)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(tt_fds); i++) {
        if (tt_fds[i].fd > 2 && tt_fds[i].fd == fd)
            return tt_fds[i].path;
    }
    return NULL;
}

static int add_to_fds(const char *path, int fd)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(tt_fds); i++) {
        if (tt_fds[i].fd > 2)
            continue;
        tt_fds[i].fd = fd;
        strncpy(tt_fds[i].path, path, 32);
        return 0;
    }
    return -1;
}

static int del_from_fds(int fd)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(tt_fds); i++) {
        if (tt_fds[i].fd <= 2)
            continue;
        if (tt_fds[i].fd == fd) {
            tt_fds[i].fd = 0;
            return 0;
        }
    }
    return -1;
}

static int vfs_test_open(int argc, char **argv)
{
    int fd, i;

    if (argc <= 0) {
        vfs_show_help();
        return -1;
    }

    for (i = 0; i < argc; i++) {
        printf("try to open %s\n", argv[i]);

        fd = path_to_fd(argv[i]);
        if (fd > 2) {
            printf("open %s at %d before\n", argv[i], fd);
            continue;
        }

        fd = open(argv[i], O_RDWR | O_CREAT, 0666);
        if (fd < 3) {
            printf("open %s failed\n", argv[i]);
            continue;
        }

        if (add_to_fds(argv[i], fd)) {
            printf("add %s to fds failed\n", argv[i]);
            close(fd);
            continue;
        }
        printf("open %s at %d ok\n", argv[i], fd);
    }
    return 0;
}

static int vfs_test_read(int argc, char **argv)
{
    int fd, i;
    ssize_t rlen;
    char buf[TEST_BUF_SIZE];

    if (argc <= 0) {
        vfs_show_help();
        return -1;
    }

    for (i = 0; i < argc; i++) {
        printf("try to read %s\n", argv[i]);

        fd = path_to_fd(argv[i]);
        if (fd < 3) {
            printf("%s had not opened, please call 'vfs open <path>' first!\n", argv[i]);
            continue;
        }

        memset(buf, 0, sizeof(buf));
        rlen = read(fd, buf, TEST_BUF_SIZE);
        if (rlen < 0) {
            printf("read %s failed\n", fd_to_path(fd));
            return -1;
        }
        printf("read %s ok\n", argv[i]);
        hexdump(buf, TEST_BUF_SIZE);
    }
    return 0;
}

static int vfs_test_write(int argc, char **argv)
{
    int fd, i, j;
    ssize_t wlen;
    char buf[TEST_BUF_SIZE];

    if (argc <= 0) {
        vfs_show_help();
        return -1;
    }

    for (i = 0; i < argc; i++) {
        printf("try to write %s\n", argv[i]);

        fd = path_to_fd(argv[i]);
        if (fd < 3) {
            printf("%s had not opened, please call 'vfs open <path>' first!\n", argv[i]);
            continue;
        }

        for (j = 0; j < TEST_BUF_SIZE; j++)
            buf[j] = (char)(j + i);

        wlen = write(fd, buf, TEST_BUF_SIZE);
        if (wlen != TEST_BUF_SIZE) {
            printf("wrtie %s failed\n", fd_to_path(fd));
            return -1;
        }
        printf("write %s ok\n", argv[i]);
        hexdump(buf, TEST_BUF_SIZE);
    }
    return 0;
}

static int vfs_test_close(int argc, char **argv)
{
    int fd, i;

    if (argc <= 0) {
        vfs_show_help();
        return -1;
    }

    for (i = 0; i < argc; i++) {
        printf("try to close %s\n", argv[i]);

        fd = path_to_fd(argv[i]);
        if (fd < 3) {
            printf("%s had not opened\n", argv[i]);
            continue;
        }
	close(fd);
        del_from_fds(fd);
        printf("close %s ok\n", argv[i]);
    }

    return 0;
}

static int vfs_test_lseek(int argc, char **argv)
{
    int fd, i, ret;

    if (argc <= 0) {
        vfs_show_help();
        return -1;
    }

    for (i = 0; i < argc; i++) {
        printf("try to lseek %s\n", argv[i]);

        fd = path_to_fd(argv[i]);
        if (fd < 3) {
            printf("%s had not opened\n", argv[i]);
            continue;
        }

        ret = lseek(fd, 0, SEEK_SET);
        if (ret < 0) {
            printf("lseek %s failed\n", fd_to_path(fd));
            return -1;
        }
        printf("lseek %s to 0, cur location %d\n", fd_to_path(fd), ret);
    }

    return 0;
}

static int vfs_test_fstat(int argc, char **argv)
{
    return 0;
}

static int vfs_test_stat(int argc, char **argv)
{
    return 0;
}
static int vfs_test_link(int argc, char **argv)
{
    return 0;
}
static int vfs_test_unlink(int argc, char **argv)
{
    return 0;
}
static int vfs_test_rename(int argc, char **argv)
{
    return 0;
}

static int vfs_auto_test(int argc, char **argv)
{
    int ret;

    if (argc <= 0) {
        vfs_show_help();
        return -1;
    }

    ret = vfs_test_open(argc, argv);
    if (ret)
        return -1;
    printf("\n");

    ret = vfs_test_write(argc, argv);
    if (ret)
        return -1;
    printf("\n");

    ret = vfs_test_lseek(argc, argv);
    if (ret)
        return -1;
    printf("\n");

    ret = vfs_test_read(argc, argv);
    if (ret)
        return -1;
    printf("\n");

    ret = vfs_test_close(argc, argv);
    if (ret)
        return -1;
    printf("\n");

    return 0;
}

static int vfs_test_list(int argc, char **argv)
{
    return 0;
}
static int vfs_test_truncate(int argc, char **argv)
{
    return 0;
}
static int vfs_test_access(int argc, char **argv)
{
    return 0;
}
static int vfs_test_fsync(int argc, char **argv)
{
    return 0;
}

static int vfs_test_init(void)
{
    int ret;

    if (init_end)
        return 0;
#ifdef CONFIG_DRIVERS_SPINOR
    ret = nor_init();
    if (ret) {
        printf("nor init failed - %d\n", ret);
        goto err;
    }
#endif

#ifdef CONFIG_COMPONENTS_AW_DEVFS
    ret = devfs_mount("/dev");
    if (ret) {
        printf("mount devfs to /dev failed - %d\n", ret);
        goto err;
    }
#endif

#ifdef CONFIG_COMPONENT_SPIFFS
    ret = spiffs_mount("/dev/UDISK", "/data", true);
    if (ret) {
        printf("register spiffs failed\n");
        goto err;
    }
#endif

    init_end = true;
    return 0;
err:
    printf("initialize vfs testcase failed\n");
    return ret;
}

struct vfs_ops {
    const char *name;
    int (*op)(int argc, char **argv);
};
#define vfs_op_node(op) {# op, vfs_test_ ## op}
struct vfs_ops vfs_ops[] = {
    vfs_op_node(open),
    vfs_op_node(read),
    vfs_op_node(write),
    vfs_op_node(lseek),
    vfs_op_node(close),
    vfs_op_node(fstat),
    vfs_op_node(stat),
    vfs_op_node(unlink),
    vfs_op_node(rename),
    vfs_op_node(list),
    vfs_op_node(truncate),
    vfs_op_node(access),
    vfs_op_node(fsync),
};

static int vfs_show_help(void)
{
    int i;

    printf("Usage:\n");
    printf("    vfs [path]:\n");
    printf("        do open,write,read,close to file on 'path'.\n");
    printf("        if no 'path' given, use '/dev/reserved0' & '/data/vfs' default\n");

    printf("    vfs <");
    for (i = 0; i < ARRAY_SIZE(vfs_ops); i++) {
        if (i + 1 < ARRAY_SIZE(vfs_ops))
            printf("%s,", vfs_ops[i].name);
        else
            printf("%s", vfs_ops[i].name);
    }
    printf("> <path>:\n");
    printf("        operate to file on 'path'\n");
    printf("    vfs list [dir]:\n");
    printf("        list all file on 'dir'.\n");
    printf("        if no 'dir' given, use '/dev' & '/data' default\n");
    printf("    vfs help:\n");
    printf("        show this help message and exit\n");
    return 0;
}

static char *def_path[] = {
#ifdef CONFIG_COMPONENT_SPIFFS
	"/data/vfs",
#endif
#ifdef CONFIG_COMPONENTS_AW_DEVFS
	"/dev/reserved0",
#endif
};

static int vfs_test(int argc, char **argv)
{
    int ret, i;

    ret = vfs_test_init();    
    if (ret)
        goto err;

    if (argc <= 1) {
        ret = vfs_auto_test(ARRAY_SIZE(def_path), def_path);
        if (ret)
            goto err;
        return 0;
    }

    if (!strcmp(argv[1], "help"))
        return vfs_show_help();

    if (!strcmp(argv[1], "list")) {
        ret = vfs_test_list(argc - 2, argv + 2);
        if (ret)
            goto err;
        return 0;
    }

    if (*argv[1] == '/') {
        ret = vfs_auto_test(argc - 1, argv + 1);
        if (ret)
            goto err;
        return 0;
    }

    for (i = 0; i < ARRAY_SIZE(vfs_ops); i++) {
        struct vfs_ops *op = &vfs_ops[i];

        if (!strcmp(argv[1], op->name)) {
            ret = op->op(argc - 2, argv + 2);
            if (ret)
                goto err;
            return 0;
        }
    }

    vfs_show_help();
    return 0;
err:
    printf("vfs test failed\n");
    return ret;
}
testcase_init(vfs_test, vfs, test vfs operation);
