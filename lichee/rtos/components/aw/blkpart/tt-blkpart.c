#include <stdint.h>
#include <blkpart.h>
#include <sunxi_hal_spinor.h>
#include <tinatest.h>
#include <awlog.h>
#include <part_efi.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static struct part *get_part(int argc, char **argv)
{
    struct part *part;

    if (argc <= 0)
        part = get_part_by_index("nor", PARTINDEX_THE_LAST);
    else
        part = get_part_by_name(argv[0]);
    if (!part) {
        printf("not found block partition %s\n", argc > 0 ? argv[1] : "");
        return NULL;
    }
    return part;
}

#define DEFAULT_ADDR 10
static int get_addr(int argc, char **argv)
{
    if (argc < 2)
        return DEFAULT_ADDR;

    return atoi(argv[1]);
}

static int blkpart_show_help(void)
{
    printf("Usage:\n");
    printf("    block [part]:\n");
    printf("        do erase/write/read to partition. If no part given,\n");
    printf("        operate the last partition.\n");
    printf("    block <read/write/erase> [part] [addr]:\n");
    printf("        operate address base on partition. If no part given,\n");
    printf("        operate the last partition. If address given, use 0\n");
    printf("        default.\n");
    printf("    block info [part]:\n");
    printf("        show one or all partition\n");
    return 0;
}

#define RW_TEST_BYTES (256 + 50)
static int blkpart_test_read(int argc, char **argv)
{
    struct part *part;
    int addr;
    char buf[RW_TEST_BYTES];
    ssize_t ret;

    part = get_part(argc, argv);
    addr = get_addr(argc, argv);
    if (!part || addr < 0)
        goto help;

    ret = blkpart_read(part, addr, RW_TEST_BYTES, buf);
    if (ret != RW_TEST_BYTES) {
        printf("read wanted %d but get %ld\n", RW_TEST_BYTES, ret);
        ret = -EIO;
        goto err;
    }

    printf("read partition %s(%s) with offset %d ok\n", part->name,
            part->devname, addr);
    hexdump(buf, RW_TEST_BYTES);
    return 0;

help:
    blkpart_show_help();
    return -ENODEV;
err:
    printf("blkpart read test failed\n");
    return ret;
}

static int blkpart_test_write(int argc, char **argv)
{
    struct part *part;
    int addr, index;
    char buf[RW_TEST_BYTES];
    ssize_t ret;

    part = get_part(argc, argv);
    addr = get_addr(argc, argv);
    if (!part || addr < 0)
        goto help;

    for (index = 0; index < RW_TEST_BYTES; index++)
        buf[index] = (char)index;

    ret = blkpart_write(part, addr, RW_TEST_BYTES, buf);
    if (ret != RW_TEST_BYTES) {
        printf("write wanted %d but get %ld\n", RW_TEST_BYTES, ret);
        ret = -EIO;
        goto err;
    }

    printf("write partition %s(%s) with offset %d ok\n", part->name,
            part->devname, addr);
    hexdump(buf, RW_TEST_BYTES);
    return 0;

help:
    blkpart_show_help();
    return -ENODEV;
err:
    printf("blkpart write test failed\n");
    return ret;
}

static int blkpart_test_erase(int argc, char **argv)
{
    struct part *part;
    int addr, ret;

    part = get_part(argc, argv);
    addr = get_addr(argc, argv);
    if (!part || addr < 0)
        goto help;

    ret = blkpart_erase(part, addr, part->blk->blk_bytes);
    if (ret)
        goto err;

    printf("blkpart test erase %s(%s) addr %d ok\n", part->name,
            part->devname, addr);
    return 0;

help:
    blkpart_show_help();
    return -ENODEV;
err:
    printf("blkpart erase test failed\n");
    return ret;
}

static int blkpart_test_info(int argc, char **argv)
{
    int i;
    const char *dest = NULL;

    if (argc)
        dest = argv[0];

    for (i = 0; i < PARTINDEX_THE_LAST; i++) {
        struct part *part = get_part_by_index("nor", i);

        if (!part)
            break;

        if (dest && (strcmp(part->name, dest) && strcmp(part->devname, dest)))
            continue;
        printf("%s(%s): bytes 0x%x off 0x%x\n", part->name, part->devname,
                part->bytes, part->off);
        if (dest)
            break;
    }
    return 0;
}

static int blkpart_auto_test(int argc, char **argv)
{
    int ret;

    ret = blkpart_test_erase(argc, argv);
    if (ret)
        return ret;
    ret = blkpart_test_write(argc, argv);
    if (ret)
        return ret;
    ret = blkpart_test_read(argc, argv);
    if (ret)
        return ret;

    return 0;
}

static int blkpart_test(int argc, char **argv)
{
    int ret;

    ret = nor_init();
    if (ret)
        goto err;

    if (argc <= 1) {
        ret = blkpart_auto_test(0, NULL);
        if (ret)
            goto err;
        return 0;
    }

    if (!strcmp(argv[1], "read")) {
        ret = blkpart_test_read(argc - 2, argv + 2);
        if (ret)
            goto err;
    } else if (!strcmp(argv[1], "write")) {
        ret = blkpart_test_write(argc - 2, argv + 2);
        if (ret)
            goto err;
    } else if (!strcmp(argv[1], "erase")) {
        ret = blkpart_test_erase(argc - 2, argv + 2);
        if (ret)
            goto err;
    } else if (!strcmp(argv[1], "info")) {
        ret = blkpart_test_info(argc - 2, argv + 2);
        if (ret)
            goto err;
    } else {
        ret = blkpart_auto_test(argc - 1, argv + 1);
        if (ret)
            goto err;
    }

    return 0;
err:
    pr_err("test blkpart failed - %d\n", ret);
    return ret;
}
testcase_init(blkpart_test, block, test blkpart based on nor flash);
