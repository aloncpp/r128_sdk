#include "aactd/communicate.h"

#include <string.h>
#include "aactd/common.h"

int aactd_com_buf_to_header(const uint8_t *buf, struct aactd_com_header *header)
{
    const uint8_t *p = buf;
    if (!buf || !header) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* flag, version, command, type */
    memcpy(header, p, 4);

    /* data_len */
    p += 4;
    header->data_len = aactd_le_buf_to_uint32(p);

    return 0;
}

int aactd_com_header_to_buf(const struct aactd_com_header *header, uint8_t *buf)
{
    uint8_t *p = buf;
    if (!buf || !header) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* flag, version, command, type */
    memcpy(p, header, 4);

    /* data_len */
    p += 4;
    aactd_uint32_to_le_buf(header->data_len, p);

    return 0;
}

void aactd_com_print_content(const struct aactd_com *com)
{
    int i;
    struct aactd_com_eq_sw_data eq_sw_data = {
        .filter_args = NULL,
    };
    struct aactd_com_drc_hw_data drc_hw_data = {
        .reg_args = NULL,
    };
    struct aactd_com_eq_hw_data eq_hw_data = {
        .reg_args = NULL,
    };
    struct aactd_com_drc_hw_long_data drc_hw_long_data = {
        .reg_args = NULL,
    };

    printf("====================\n");
    printf("Header:\n");
    printf("  flag: 0x%x\n", com->header.flag);
    printf("  version: %u\n", com->header.version);
    printf("  command: %u\n", com->header.command);
    printf("  type: %u\n", com->header.type);
    printf("  data_len: %u\n", com->header.data_len);

    printf("Data:\n");
    switch (com->header.type) {
    case EQ_SW:
        eq_sw_data.global_enabled = com->data[0];
        eq_sw_data.filter_num = com->data[1];
        printf("  global_enabled: %u\n", eq_sw_data.global_enabled);
        printf("  filter_num: %u\n", eq_sw_data.filter_num);
        for (i = 0; i < eq_sw_data.filter_num; ++i) {
            struct aactd_com_eq_sw_filter_arg arg;
            aactd_com_eq_sw_buf_to_filter_arg(
                    com->data + 2 + i * sizeof(struct aactd_com_eq_sw_filter_arg),
                    &arg);
            printf("  [%02d] type: %u,  freq: %d,  gain: %d,  Q: %.2f,  enabled: %d\n",
                    i + 1, arg.type, arg.frequency, arg.gain,
                    (float)(arg.quality) / 100, arg.enabled);
        }
        break;

    case DRC_HW:
		#if defined(CONFIG_ARCH_SUN20IW2)
		drc_hw_long_data.reg_num = aactd_le_buf_to_uint16(com->data);
        printf("  reg_num: %u\n", drc_hw_long_data.reg_num);
        for (i = 0; i < drc_hw_long_data.reg_num; ++i) {
            struct aactd_com_drc_hw_reg_long_arg arg;
            aactd_com_drc_hw_buf_to_reg_long_arg(
                    com->data + 2 + i * sizeof(struct aactd_com_drc_hw_reg_long_arg),
                    &arg);
            printf("  [0x%08x]: 0x%08x\n", arg.offset, arg.value);
        }
		#elif defined(CONFIG_ARCH_SUN8IW18) || defined(CONFIG_ARCH_SUN8IW19) || defined(CONFIG_ARCH_SUN8IW20)
        drc_hw_data.reg_num = aactd_le_buf_to_uint16(com->data);
        printf("  reg_num: %u\n", drc_hw_data.reg_num);
        for (i = 0; i < drc_hw_data.reg_num; ++i) {
            struct aactd_com_drc_hw_reg_arg arg;
            aactd_com_drc_hw_buf_to_reg_arg(
                    com->data + 2 + i * sizeof(struct aactd_com_drc_hw_reg_arg),
                    &arg);
            printf("  [0x%08x]: 0x%08x\n", arg.offset, arg.value);
        }
		#endif
        break;
    case EQ_HW:
        eq_hw_data.reg_num = aactd_le_buf_to_uint16(com->data);
        printf("  reg_num: %u\n", eq_hw_data.reg_num);
        for (i = 0; i < eq_hw_data.reg_num; ++i) {
            struct aactd_com_eq_hw_reg_arg arg;
            aactd_com_eq_hw_buf_to_reg_arg(
                    com->data + 2 + i * sizeof(struct aactd_com_eq_hw_reg_arg),
                    &arg);
            printf("  [0x%08x]: 0x%08x\n", arg.offset, arg.value);
        }
        break;
	case DRC3_HW:
		#if defined(CONFIG_ARCH_SUN20IW2)
		drc_hw_long_data.reg_num = aactd_le_buf_to_uint16(com->data);
        printf("  reg_num: %u\n", drc_hw_long_data.reg_num);
        for (i = 0; i < drc_hw_long_data.reg_num; ++i) {
            struct aactd_com_drc_hw_reg_long_arg arg;
            aactd_com_drc_hw_buf_to_reg_long_arg(
                    com->data + 2 + i * sizeof(struct aactd_com_drc_hw_reg_long_arg),
                    &arg);
            printf("  [0x%08x]: 0x%08x\n", arg.offset, arg.value);
        }
		#endif
		break;

    default:
        break;
    }

    printf("Checksum: 0x%x\n", com->checksum);
    printf("====================\n");
}

void aactd_com_copy(const struct aactd_com *src, struct aactd_com *dst)
{
    memcpy(&dst->header, &src->header, sizeof(struct aactd_com_header));
    memcpy(dst->data, src->data, src->header.data_len);
    dst->checksum = src->checksum;
}

int aactd_com_eq_sw_buf_to_filter_arg(
        const uint8_t *buf, struct aactd_com_eq_sw_filter_arg *arg)
{
    const uint8_t *p = buf;
    if (!buf || !arg) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* type */
    arg->type = *p;

    /* frequency */
    p += 1;
    arg->frequency = aactd_le_buf_to_int32(p);

    /* gain */
    p += 4;
    arg->gain = aactd_le_buf_to_int32(p);

    /* quality */
    p += 4;
    arg->quality = aactd_le_buf_to_int32(p);

    /* enabled */
    p += 4;
    arg->enabled = *p;

    return 0;
}

int aactd_com_eq_sw_filter_arg_to_buf(
        const struct aactd_com_eq_sw_filter_arg *arg, uint8_t *buf)
{
    uint8_t *p = buf;
    if (!buf || !arg) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* type */
    *p = arg->type;

    /* frequency */
    p += 1;
    aactd_int32_to_le_buf(arg->frequency, p);

    /* gain */
    p += 4;
    aactd_int32_to_le_buf(arg->gain, p);

    /* quality */
    p += 4;
    aactd_int32_to_le_buf(arg->quality, p);

    /* enabled */
    p += 4;
    *p = arg->enabled;

    return 0;
}

int aactd_com_eq_sw_buf_to_data(
        const uint8_t *buf, struct aactd_com_eq_sw_data *data)
{
    const uint8_t *p = buf;
    int i;

    if (!buf || !data) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* global_enabled */
    data->global_enabled = *p;

    /* filter_num */
    p += 1;
    data->filter_num = *p;

    /* filter_args */
    p += 1;
    for (i = 0; i < data->filter_num; ++i) {
        aactd_com_eq_sw_buf_to_filter_arg(p, &data->filter_args[i]);
        p += sizeof(struct aactd_com_eq_sw_filter_arg);
    }

    return 0;
}

int aactd_com_eq_sw_data_to_buf(
        const struct aactd_com_eq_sw_data *data, uint8_t *buf)
{
    uint8_t *p = buf;
    int i;

    if (!buf || !data) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* global_enabled */
    *p = data->global_enabled;

    /* filter_num */
    p += 1;
    *p = data->filter_num;

    /* filter_args */
    p += 1;
    for (i = 0; i < data->filter_num; ++i) {
        aactd_com_eq_sw_filter_arg_to_buf(&data->filter_args[i], p);
        p += sizeof(struct aactd_com_eq_sw_filter_arg);
    }

    return 0;
}

int aactd_com_drc_hw_buf_to_reg_arg(
        const uint8_t *buf, struct aactd_com_drc_hw_reg_arg *arg)
{
    const uint8_t *p = buf;
    if (!buf || !arg) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* offset */
    arg->offset = aactd_le_buf_to_uint32(p);

    /* value */
    p += 4;
    arg->value = aactd_le_buf_to_uint32(p);

    return 0;
}

int aactd_com_drc_hw_reg_arg_to_buf(
        const struct aactd_com_drc_hw_reg_arg *arg, uint8_t *buf)
{
    uint8_t *p = buf;
    if (!buf || !arg) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* offset */
    aactd_uint32_to_le_buf(arg->offset, p);

    /* value */
    p += 4;
    aactd_uint32_to_le_buf(arg->value, p);

    return 0;
}

int aactd_com_drc_hw_buf_to_data(
        const uint8_t *buf, struct aactd_com_drc_hw_data *data)
{
    const uint8_t *p = buf;
    int i;

    if (!buf || !data) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* reg_num */
    data->reg_num = aactd_le_buf_to_uint16(p);

    /* reg_args */
    p += 2;
    for (i = 0; i < data->reg_num; ++i) {
        aactd_com_drc_hw_buf_to_reg_arg(p, &data->reg_args[i]);
        p += sizeof(struct aactd_com_drc_hw_reg_arg);
    }

    return 0;
}

int aactd_com_drc_hw_data_to_buf(
        const struct aactd_com_drc_hw_data *data, uint8_t *buf)
{
    uint8_t *p = buf;
    int i;

    if (!buf || !data) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* reg_num */
    aactd_uint16_to_le_buf(data->reg_num, p);

    /* reg_args */
    p += 2;
    for (i = 0; i < data->reg_num; ++i) {
        aactd_com_drc_hw_reg_arg_to_buf(&data->reg_args[i], p);
        p += sizeof(struct aactd_com_drc_hw_reg_arg);
    }

    return 0;
}

int aactd_com_eq_hw_reg_arg_to_buf(
        const struct aactd_com_eq_hw_reg_arg *arg, uint8_t *buf)
{
    uint8_t *p = buf;
    if (!buf || !arg) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* offset */
    aactd_uint64_to_le_buf(arg->offset, p);

    /* value */
    p += 8;
    aactd_uint64_to_le_buf(arg->value, p);

    return 0;
}

int aactd_com_eq_hw_buf_to_reg_arg(
        const uint8_t *buf, struct aactd_com_eq_hw_reg_arg *arg)
{
    const uint8_t *p = buf;
    if (!buf || !arg) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* offset */
    arg->offset = aactd_le_buf_to_uint64(p);

    /* value */
    p += 8;
    arg->value = aactd_le_buf_to_uint64(p);

    return 0;
}

int aactd_com_eq_hw_buf_to_data(
        const uint8_t *buf, struct aactd_com_eq_hw_data *data)
{
    const uint8_t *p = buf;
    int i;

    if (!buf || !data) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* reg_num */
    data->reg_num = aactd_le_buf_to_uint16(p);

    /* reg_args */
    p += 2;
    for (i = 0; i < data->reg_num; ++i) {
        aactd_com_eq_hw_buf_to_reg_arg(p, &data->reg_args[i]);
        p += sizeof(struct aactd_com_eq_hw_reg_arg);
    }

    return 0;
}

int aactd_com_eq_hw_data_to_buf(
        const struct aactd_com_eq_hw_data *data, uint8_t *buf)
{
    uint8_t *p = buf;
    int i;

    if (!buf || !data) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* reg_num */
    aactd_uint16_to_le_buf(data->reg_num, p);

    /* reg_args */
    p += 2;
    for (i = 0; i < data->reg_num; ++i) {
        aactd_com_eq_hw_reg_arg_to_buf(&data->reg_args[i], p);
        p += sizeof(struct aactd_com_eq_hw_reg_arg);
    }

    return 0;
}


int aactd_com_drc_hw_reg_long_arg_to_buf(
        const struct aactd_com_drc_hw_reg_long_arg *arg, uint8_t *buf)
{
    uint8_t *p = buf;
    if (!buf || !arg) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* offset */
    aactd_uint64_to_le_buf(arg->offset, p);

    /* value */
    p += 8;
    aactd_uint64_to_le_buf(arg->value, p);

    return 0;
}

int aactd_com_drc_hw_buf_to_reg_long_arg(
        const uint8_t *buf, struct aactd_com_drc_hw_reg_long_arg *arg)
{
    const uint8_t *p = buf;
    if (!buf || !arg) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* offset */
    arg->offset = aactd_le_buf_to_uint64(p);

    /* value */
    p += 8;
    arg->value = aactd_le_buf_to_uint64(p);

    return 0;
}

int aactd_com_drc_hw_buf_to_long_data(
        const uint8_t *buf, struct aactd_com_drc_hw_long_data *data)
{
    const uint8_t *p = buf;
    int i;

    if (!buf || !data) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* reg_num */
    data->reg_num = aactd_le_buf_to_uint16(p);

    /* reg_args */
    p += 2;
    for (i = 0; i < data->reg_num; ++i) {
        aactd_com_drc_hw_buf_to_reg_long_arg(p, &data->reg_args[i]);
        p += sizeof(struct aactd_com_drc_hw_reg_long_arg);
    }

    return 0;
}

int aactd_com_drc_hw_long_data_to_buf(
        const struct aactd_com_drc_hw_long_data *data, uint8_t *buf)
{
    uint8_t *p = buf;
    int i;

    if (!buf || !data) {
        aactd_error("Invalid arguments\n");
        return -1;
    }

    /* reg_num */
    aactd_uint16_to_le_buf(data->reg_num, p);

    /* reg_args */
    p += 2;
    for (i = 0; i < data->reg_num; ++i) {
        aactd_com_drc_hw_reg_long_arg_to_buf(&data->reg_args[i], p);
        p += sizeof(struct aactd_com_drc_hw_reg_long_arg);
    }

    return 0;
}


