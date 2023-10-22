#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <hal_timer.h>
#include <sunxi_hal_common.h>

#ifdef CONFIG_COMPONENTS_AW_BLKPART
#include <blkpart.h>
#include <part_efi.h>
#endif

#include "sunxi_amp.h"

#include <hal_cmd.h>
#include <hal_log.h>
#include <hal_cache.h>

#define FLASHC_FMT(fmt) "flashc_stub: "fmt
#define FLASHC_STUB_ERR(fmt, arg...) hal_log_err(FLASHC_FMT(fmt), ##arg)
#define FLASHC_STUB_INFO(fmt, arg...) hal_log_info(FLASHC_FMT(fmt), ##arg)

MAYBE_STATIC int nor_read(unsigned int addr, char *buf, unsigned int size)
{
    int ret;
    void *args[3] = {0};
    char *buffer = amp_align_malloc(size);
    if (!buffer)
    {
        return -1;
    }
    args[0] = (void *)(unsigned long)addr;
    args[1] = buffer;
    args[2] = (void *)(unsigned long)size;
    hal_dcache_invalidate((unsigned long)buffer, size);
    ret = func_stub(RPCCALL_FLASHC(nor_read), 1, ARRAY_SIZE(args), args);
    hal_dcache_invalidate((unsigned long)buffer, size);
    memcpy(buf, buffer, size);
    amp_align_free(buffer);
    return ret;
}

MAYBE_STATIC int nor_write(unsigned int addr, char *buf, unsigned int size)
{
    int ret;
    void *args[3] = {0};
    char *buffer = amp_align_malloc(size);
    if (!buffer)
    {
        return -1;
    }
    args[0] = (void *)(unsigned long)addr;
    args[1] = buffer;
    args[2] = (void *)(unsigned long)size;
    memcpy(buffer, buf, size);
    hal_dcache_clean((unsigned long)buffer, size);
    ret = func_stub(RPCCALL_FLASHC(nor_write), 1, ARRAY_SIZE(args), args);
    amp_align_free(buffer);
    return ret;
}

MAYBE_STATIC int nor_erase(unsigned int addr, unsigned int size)
{
    void *args[2] = {0};
    args[0] = (void *)(unsigned long)addr;
    args[1] = (void *)(unsigned long)size;
    return func_stub(RPCCALL_FLASHC(nor_erase), 1, ARRAY_SIZE(args), args);
}

MAYBE_STATIC int nor_ioctrl(int cmd, void *buffer, unsigned int size)
{
    int ret;
    void *args[3] = {0};
    args[0] = (void *)(unsigned long)cmd;
    args[1] = buffer;
    args[2] = (void *)(unsigned long)size;
    hal_dcache_invalidate((unsigned long)buffer, size);
    ret = func_stub(RPCCALL_FLASHC(nor_ioctrl), 1, ARRAY_SIZE(args), args);
    hal_dcache_invalidate((unsigned long)buffer, size);
    return ret;
}

#ifdef CONFIG_COMPONENTS_AW_BLKPART
struct syspart {
    char name[MAX_BLKNAME_LEN];
    u32 bytes;
};

static const struct syspart syspart[] = {
#ifdef CONFIG_ARCH_SUN20IW2P1
    {"boot0", 64 * 1024},
    {"boot0-bk", 64 * 1024},
    {"gpt", 16 * 1024},
#else
    /* contain boot0 and gpt, gpt offset is (128-16)k */
    {"boot0", CONFIG_COMPONENTS_AW_BLKPART_LOGICAL_OFFSET * 1024},
#endif
};

static struct blkpart norblk;

static int nor_get_gpt(char *buf, int len)
{
    if (len < GPT_TABLE_SIZE) {
        FLASHC_STUB_ERR("buf too small for gpt\n");
        return -EINVAL;
    }

    return nor_read(GPT_ADDRESS, buf, GPT_TABLE_SIZE);
}
#endif

#ifdef CONFIG_RESERVE_IMAGE_PART

typedef struct sbrom_toc1_head_info
{
    char name[16]	;	//user can modify
    uint32_t  magic	;	//must equal TOC_U32_MAGIC
    uint32_t  add_sum	;

    uint32_t  serial_num	;	//user can modify
    uint32_t  status		;	//user can modify,such as TOC_MAIN_INFO_STATUS_ENCRYP_NOT_USED

    uint32_t  items_nr;	//total entry number
    uint32_t  valid_len;
    uint32_t  version_main;	//only one byte
    uint32_t  version_sub;   //two bytes
    uint32_t  reserved[3];	//reserved for future

    uint32_t  end;
}__attribute__((aligned(1))) sbrom_toc1_head_info_t;

/*
 * get_rtos_toc_package_size: get rtos toc package size
 * @param gpt_buf: gpt table buf
 * @param name   : gpt entry name
 * @param page_bytes: page size
 * @return value : toc pageage size
 */
int get_rtos_toc_package_size(void *gpt_buf, char *name, int page_bytes)
{
    unsigned int len = 0x0;
    unsigned int sectors = 0;
    unsigned int start_sector = 0;
    int toc_offset = 0;
    int ret = -1;

    char *buf = malloc(page_bytes);
    if (!buf)
        return -1;
    memset(buf, 0, page_bytes);

    ret = get_part_info_by_name(gpt_buf, name, &start_sector, &sectors);
    if (ret)
        goto out;

    toc_offset = start_sector << 9;
    ret = nor_read(toc_offset, buf, page_bytes);
    if (ret)
        goto out;

    sbrom_toc1_head_info_t *head = (sbrom_toc1_head_info_t *)buf;
    if (strncmp(head->name, "sunxi-", 6))
        goto out;

#define ALIGN_UP(size, align) (((size) + (align) - 1) & ~((align) - 1))
    len = ALIGN_UP(head->valid_len, page_bytes);

out:
    free(buf);

    return len;
}

/*
 * get_rtos_offset: get rtos offset
 * @param gpt_buf: gpt table buf
 * @param name   : gpt entry name
 * @return value : rtos part offset
 */
int get_rtos_offset(void *gpt_buf, char *name)
{
    unsigned int sectors = 0;
    unsigned int start_sector = 0;
    int ret = -1;

    ret = get_part_info_by_name(gpt_buf, name, &start_sector, &sectors);
    if (ret)
        goto out;

    ret = start_sector << 9;
out:

    return ret;
}

#endif

enum {
    NOR_IOCTRL_GET_PAGE_SIZE = 0,
    NOR_IOCTRL_GET_BLK_SIZE,
    NOR_IOCTRL_GET_TOTAL_SIZE,
};

int nor_blkpart_init(void)
{
    int ret, index;
    char *gpt_buf;
    struct gpt_part *gpt_part;
    struct part *part;

    __attribute__((aligned(64))) unsigned int page_size_array[CACHELINE_LEN / sizeof(int)];
    __attribute__((aligned(64))) unsigned int blk_size_array[CACHELINE_LEN / sizeof(int)];
    __attribute__((aligned(64))) unsigned int total_size_array[CACHELINE_LEN / sizeof(int)];

    unsigned int page_size = 0;
    unsigned int blk_size = 0;
    unsigned int total_size = 0;

    gpt_buf = amp_align_malloc(GPT_TABLE_SIZE);
    if (!gpt_buf) {
        ret = -ENOMEM;
        goto err;
    }
    memset(gpt_buf, 0, GPT_TABLE_SIZE);

    ret = nor_get_gpt(gpt_buf, GPT_TABLE_SIZE);
    if (ret) {
        FLASHC_STUB_ERR("get gpt from nor flash failed - %d\n", ret);
        goto err;
    }

    nor_ioctrl(NOR_IOCTRL_GET_PAGE_SIZE, page_size_array, sizeof(page_size_array));
    nor_ioctrl(NOR_IOCTRL_GET_BLK_SIZE, blk_size_array, sizeof(blk_size_array));
    nor_ioctrl(NOR_IOCTRL_GET_TOTAL_SIZE, total_size_array, sizeof(total_size_array));
    page_size = page_size_array[0];
    blk_size = blk_size_array[0];
    total_size = total_size_array[0];

    memset(&norblk, 0, sizeof(struct blkpart));
    norblk.name = "nor";
    norblk.page_bytes = page_size;
#ifdef CONFIG_DRIVERS_SPINOR_CACHE
    norblk.erase = nor_cache_erase;
    norblk.program = nor_cache_write;
    norblk.read = nor_cache_read;
    norblk.sync = nor_cache_sync;
#else
    norblk.erase = nor_erase;
    norblk.program = nor_write;
    norblk.read = nor_read;
    norblk.sync = NULL;
#endif
    norblk.noncache_erase = nor_erase;
    norblk.noncache_program = nor_write;
    norblk.noncache_read = nor_read;
    norblk.total_bytes = total_size;
    norblk.blk_bytes = blk_size;

    ret = gpt_part_cnt(gpt_buf);
    if (ret < 0) {
        FLASHC_STUB_ERR("get part count from gpt failed\n");
#ifdef CONFIG_COMPONENTS_AW_BLKPART_NO_GPT
        printf("when no gpt, hardcode 2 part, 0-2M:rtos 2M-end:UDISK\n");
        norblk.n_parts = 2;
        norblk.parts = malloc(norblk.n_parts * sizeof(struct part));
        part = &norblk.parts[0];
        snprintf(part->name, MAX_BLKNAME_LEN, "%s", "rtos");
        part->bytes = 2*1024*1024;
        part->off = 0;
        part = &norblk.parts[1];
        part->bytes = norblk.total_bytes - 2*1024*1024;
        part->off = 2*1024*1024;
        snprintf(part->name, MAX_BLKNAME_LEN, "%s", "UDISK");
        ret = add_blkpart(&norblk);
        if (ret)
                goto free_parts;
        return 0;
#else
        goto err;
#endif
    }
#ifdef CONFIG_RESERVE_IMAGE_PART
    norblk.n_parts = ret + ARRAY_SIZE(syspart) + 2;
#else
    norblk.n_parts = ret + ARRAY_SIZE(syspart);
#endif
    norblk.parts = malloc(norblk.n_parts * sizeof(struct part));
    if (!norblk.parts)
        goto err;
    FLASHC_STUB_INFO("total %u part\n", norblk.n_parts);

    for (index = 0; index < ARRAY_SIZE(syspart); index++) {
        part = &norblk.parts[index];
        part->bytes = syspart[index].bytes;
        part->off = BLKPART_OFF_APPEND;
        strcpy(part->name, syspart[index].name);
    }

    foreach_gpt_part(gpt_buf, gpt_part) {
        part = &norblk.parts[index++];
        part->bytes = gpt_part->sects << SECTOR_SHIFT;
        part->off = BLKPART_OFF_APPEND;
        snprintf(part->name, MAX_BLKNAME_LEN, "%s", gpt_part->name);
#ifdef CONFIG_RESERVE_IMAGE_PART
        if (!strcmp("rtosA", part->name) || !strcmp("rtosB", part->name)) {
            int rtos_index = index - 1;
            struct part *last_part = part;
            int toc_package_size = get_rtos_toc_package_size(gpt_buf, last_part->name, norblk.page_bytes);
            int rtos_offset = get_rtos_offset(gpt_buf, last_part->name);
            if (toc_package_size > 0 && rtos_offset > 0) {
                part = &norblk.parts[index++];
                part->bytes = norblk.parts[rtos_index].bytes - toc_package_size;
                part->off = rtos_offset + toc_package_size;
                if (!strcmp("rtosA", last_part->name))
                    snprintf(part->name, MAX_BLKNAME_LEN, "%s", "reserveA");
                else
                    snprintf(part->name, MAX_BLKNAME_LEN, "%s", "reserveB");
            } else {
                norblk.n_parts --;
            }
        }
#endif
    }
    norblk.parts[--index].bytes = BLKPART_SIZ_FULL;

    ret = add_blkpart(&norblk);
    if (ret)
        goto free_parts;

    /* check bytes align */
    for (index = 0; index < norblk.n_parts; index++) {
        part = &norblk.parts[index];
        if (part->bytes % blk_size) {
            FLASHC_STUB_ERR("part %s with bytes %u should align to block size %u\n",
                    part->name, part->bytes, blk_size);
            goto del_blk;
        }
    }

    amp_align_free(gpt_buf);
    return 0;

del_blk:
    del_blkpart(&norblk);
free_parts:
    free(norblk.parts);
err:
    amp_align_free(gpt_buf);
    FLASHC_STUB_ERR("init blkpart for nor failed - %d\n", ret);
    return ret;
}

MAYBE_STATIC int hal_flashc_init(void)
{
    return nor_blkpart_init();
}


