#ifndef __AACTD_COMMUNICATE_H__
#define __AACTD_COMMUNICATE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AACTD_COM_HEADER_FLAG 0xAA

#define AACTD_COM_HEADER_FLAG_V2 0xBB

struct aactd_com_header {
    uint8_t flag;
    uint8_t version;
    uint8_t command;
    uint8_t type;
    uint32_t data_len;
} __attribute__((packed));

struct aactd_com {
    struct aactd_com_header header;
    uint8_t *data;
    uint8_t checksum;
} __attribute__((packed));

enum aactd_com_command {
    CMD_READ = 0,
    CMD_WRITE = 1,
};

enum aactd_com_type {
    AACTD_TYPE_RESERVED = 0,
    EQ_SW = 1,
    DRC_HW = 2,
    EQ_HW = 3,
    DRC3_HW = 4,
};

int aactd_com_buf_to_header(const uint8_t *buf, struct aactd_com_header *header);
int aactd_com_header_to_buf(const struct aactd_com_header *header, uint8_t *buf);

void aactd_com_print_content(const struct aactd_com *com);
void aactd_com_copy(const struct aactd_com *src_com, struct aactd_com *dst_com);

/* ============ EQ_SW ============ */

struct aactd_com_eq_sw_filter_arg {
    uint8_t type;
    int32_t frequency;
    int32_t gain;
    int32_t quality;    /* the original quality value (float) * 100 */
    uint8_t enabled;
} __attribute__((packed));

struct aactd_com_eq_sw_data {
    uint8_t global_enabled;
    uint8_t filter_num;
    struct aactd_com_eq_sw_filter_arg *filter_args;
} __attribute__((packed));

int aactd_com_eq_sw_buf_to_filter_arg(
        const uint8_t *buf, struct aactd_com_eq_sw_filter_arg *arg);
int aactd_com_eq_sw_filter_arg_to_buf(
        const struct aactd_com_eq_sw_filter_arg *arg, uint8_t *buf);

int aactd_com_eq_sw_buf_to_data(
        const uint8_t *buf, struct aactd_com_eq_sw_data *data);
int aactd_com_eq_sw_data_to_buf(
        const struct aactd_com_eq_sw_data *data, uint8_t *buf);

/* ============ DRC_HW ============ */

struct aactd_com_drc_hw_reg_arg {
    uint32_t offset;
    uint32_t value;
} __attribute__((packed));

struct aactd_com_drc_hw_data {
    uint16_t reg_num;
    struct aactd_com_drc_hw_reg_arg *reg_args;
} __attribute__((packed));

int aactd_com_drc_hw_buf_to_reg_arg(
        const uint8_t *buf, struct aactd_com_drc_hw_reg_arg *arg);
int aactd_com_drc_hw_reg_arg_to_buf(
        const struct aactd_com_drc_hw_reg_arg *arg, uint8_t *buf);

int aactd_com_drc_hw_buf_to_data(
        const uint8_t *buf, struct aactd_com_drc_hw_data *data);
int aactd_com_drc_hw_data_to_buf(
        const struct aactd_com_drc_hw_data *data, uint8_t *buf);

/* ============ EQ_HW ============ */

struct aactd_com_eq_hw_reg_arg {
	uint64_t offset;
	uint64_t value;
} __attribute__((packed));

struct aactd_com_eq_hw_data {
	uint16_t reg_num;
	struct aactd_com_eq_hw_reg_arg *reg_args;
} __attribute__((packed));

int aactd_com_eq_hw_reg_arg_to_buf(
        const struct aactd_com_eq_hw_reg_arg *arg, uint8_t *buf);

int aactd_com_eq_hw_buf_to_reg_arg(
        const uint8_t *buf, struct aactd_com_eq_hw_reg_arg *arg);

int aactd_com_eq_hw_buf_to_data(
			const uint8_t *buf, struct aactd_com_eq_hw_data *data);

int aactd_com_eq_hw_data_to_buf(
			const struct aactd_com_eq_hw_data *data, uint8_t *buf);

/* ============ DRC_LONG_HW ============ */

struct aactd_com_drc_hw_reg_long_arg {
	uint64_t offset;
	uint64_t value;
} __attribute__((packed));

struct aactd_com_drc_hw_long_data {
	uint16_t reg_num;
	struct aactd_com_drc_hw_reg_long_arg *reg_args;
} __attribute__((packed));

int aactd_com_drc_hw_buf_to_reg_long_arg(
		const uint8_t *buf, struct aactd_com_drc_hw_reg_long_arg *arg);
int aactd_com_drc_hw_reg_long_arg_to_buf(
		const struct aactd_com_drc_hw_reg_long_arg *arg, uint8_t *buf);

int aactd_com_drc_hw_buf_to_long_data(
		const uint8_t *buf, struct aactd_com_drc_hw_long_data *data);
int aactd_com_drc_hw_long_data_to_buf(
		const struct aactd_com_drc_hw_long_data *data, uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif /* ifndef __AACTD_COMMUNICATE_H__ */
