#define pr_fmt(fmt) "part-efi: " fmt

#include <stdint.h>
#include <string.h>
#include <awlog.h>
#include <stdlib.h>
#include <blkpart.h>

#include <part_efi.h>

static inline bool is_gpt_valid(void *gpt_buf)
{
    gpt_header *gpt_head = (gpt_header *)(gpt_buf + GPT_HEAD_OFFSET);

    if (gpt_head->signature != GPT_HEADER_SIGNATURE) {
        pr_err("invalid GPT partition table\n");
	hexdump(gpt_buf, 128);
        return false;
    }
    return true;
}

int show_gpt_part(void *gpt_buf)
{
    gpt_header *header = (gpt_header *)(gpt_buf + GPT_HEAD_OFFSET);
    gpt_entry *entry = (gpt_entry *)(gpt_buf + GPT_ENTRY_OFFSET);
    int i;

    if (!is_gpt_valid(gpt_buf))
        return -1;

    printf("(GPT) Partition Table:\n");
    printf("\tname\toff_sect\tsects\n");
    for(i = 0; i < header->num_partition_entries; i++)
        printf("\t%s\t%lx\t%lx\n", (char *)entry[i].partition_name,
                (long)entry[i].starting_lba, (long)entry[i].ending_lba);

    return 0;
}

int get_part_info_by_name(void *gpt_buf, char* name, unsigned int* start_sector, unsigned int* sectors)
{
    int i, j;

    char char8_name[PARTNAME_SZ] = {0};
    gpt_header *gpt_head = (gpt_header*)(gpt_buf + GPT_HEAD_OFFSET);
    gpt_entry *entry = (gpt_entry*)(gpt_buf + GPT_ENTRY_OFFSET);

    if (!is_gpt_valid(gpt_buf))
        return -1;

    for(i = 0; i < gpt_head->num_partition_entries; i++ ){
        for(j = 0; j < PARTNAME_SZ; j++ ) {
            char8_name[j] = (char)(entry[i].partition_name[j]);
        }
        if(!strcmp(name,char8_name)){
            *start_sector = (uint)entry[i].starting_lba;
            *sectors = ((uint)entry[i].ending_lba + 1) - (uint)entry[i].starting_lba;
            return 0;
        }
    }

    return -1;
}

int gpt_part_cnt(void *gpt_buf)
{
    gpt_header *header = (gpt_header *)(gpt_buf + GPT_HEAD_OFFSET);

    if (!is_gpt_valid(gpt_buf))
        return -1;

    return header->num_partition_entries;
}

static void gpt_part_name_copy(char *dest, efi_char16_t *src, int len)
{
    int i;

    for (i = 0; i < len; i++)
        dest[i] = (char)(src[i]);
}

struct gpt_part *first_gpt_part(void *gpt_buf)
{
    struct gpt_part *part;
    gpt_header *header;
    gpt_entry *entry;

    if (!is_gpt_valid(gpt_buf))
        return NULL;

    header = (gpt_header *)(gpt_buf + GPT_HEAD_OFFSET);
    if (header->num_partition_entries == 0)
        return NULL;

    part = malloc(sizeof(*part));
    if (!part)
        return NULL;

    entry = (gpt_entry *)(gpt_buf + GPT_ENTRY_OFFSET);
    gpt_part_name_copy(part->name, entry[0].partition_name, PARTNAME_SZ);
    part->header = header;
    part->entry = entry;
    part->index = 0;
    part->off_sects = (u32)entry[0].starting_lba + GPT_START_MAPPING;
    part->sects = ((u32)entry[0].ending_lba + 1) - (u32)entry[0].starting_lba;
    return part;
}

struct gpt_part *next_gpt_part(struct gpt_part *part)
{
    gpt_header *header = part->header;
    gpt_entry *entry = part->entry;
    int i;

    part->index += 1;
    if (part->index >= header->num_partition_entries)
        goto end;

    i = part->index;
    gpt_part_name_copy(part->name, entry[i].partition_name, PARTNAME_SZ);
    part->off_sects = entry[i].starting_lba + GPT_START_MAPPING;
    part->sects = entry[i].ending_lba + 1 - entry[i].starting_lba;
    return part;

end:
    /**
     * the part is allocate by first_gpt_part(), should be free when there
     * is no any other partition
     */
    free(part);
    return NULL;

}
