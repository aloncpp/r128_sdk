#ifndef __DSP_HEAD_H
#define __DSP_HEAD_H

#define RTOS_MAGIC		"freertos"
#define MAGIC_SIZE		8
#define RTOS_VERSION		1
#define PADDING_LEN		(512)


struct dts_uart_pin_msg_t {
	unsigned int port;
	unsigned int port_num;
	unsigned int mul_sel;
};

struct dts_uart_msg_t {
	unsigned int status;
	unsigned int uart_port;
	struct dts_uart_pin_msg_t uart_pin_msg[2];
};

struct dts_gpio_int_t {
	unsigned int gpio_a;
	unsigned int gpio_b;
	unsigned int gpio_c;
	unsigned int gpio_d;
	unsigned int gpio_e;
	unsigned int gpio_f;
	unsigned int gpio_g;
};

struct dts_sharespace_t {
	/* sharespace status */
	unsigned int status;
	/* dsp write space msg */
	unsigned int dsp_write_addr;
	unsigned int dsp_write_size;
	/* arm write space msg */
	unsigned int arm_write_addr;
	unsigned int arm_write_size;
	/* dsp log space msg */
	unsigned int dsp_log_addr;
	unsigned int dsp_log_size;
};

/* dts msg about dsp */
struct dts_msg_t {
	/* dsp status */
	unsigned int dsp_status;
	/* uart */
	struct dts_uart_msg_t uart_msg;
	/* gpio int */
	struct dts_gpio_int_t gpio_int;
	/* shacespace */
	struct dts_sharespace_t dts_sharespace;
};

/*the size is 48*/
typedef struct rtos_img_hdr_t {
	unsigned int  jump_instruction;   /* one intruction jumping to real code */
	unsigned char magic[MAGIC_SIZE];  /* ="freertos"*/
	unsigned int  check_sum;          /* generated by PC */
	unsigned int  image_size;        /* the size of image*/
	unsigned int  header_size;       /* the size of header */
	unsigned int  rtos_version;		 /* rtos version */
	unsigned int  dram_size;		  /* dram size in K*/
	unsigned int  rtos_run_addr;	/*rtos load addr*/
	unsigned int  dtb_addr;			/*dtb load addr*/
	unsigned int  dtb_size;			/*the size of dtb*/
	struct dts_msg_t dts_msg;
	unsigned char reserved[4];
} rtos_img_hdr;

typedef struct box_start_os_cfg {
	unsigned int used;
	unsigned int start_type;
	unsigned int irkey_used;
	unsigned int pmukey_used;
	unsigned int pmukey_num;
	unsigned int led_power;
	unsigned int led_state;
} box_start_os_cfg_t;

/*
typedef struct ir_code {
	unsigned int key_code;
	unsigned int addr_code;
} ir_code_t;

#define IR_NUM_KEY_SUP                  (64)
typedef struct ir_key {
	unsigned int num;
	ir_code_t ir_code_depot[IR_NUM_KEY_SUP];
} ir_key_t;
*/
struct dram_para_32 {
	/* normal configuration */
	unsigned int dram_clk;
	unsigned int dram_type;
	unsigned int dram_zq;
	unsigned int dram_odt_en;

	/* control configuration */
	unsigned int dram_para1;
	unsigned int dram_para2;

	/* timing configuration */
	unsigned int dram_mr0;
	unsigned int dram_mr1;
	unsigned int dram_mr2;
	unsigned int dram_mr3;
	unsigned int dram_tpr0;
	unsigned int dram_tpr1;
	unsigned int dram_tpr2;
	unsigned int dram_tpr3;
	unsigned int dram_tpr4;
	unsigned int dram_tpr5;
	unsigned int dram_tpr6;
	unsigned int dram_tpr7;
	unsigned int dram_tpr8;
	unsigned int dram_tpr9;
	unsigned int dram_tpr10;
	unsigned int dram_tpr11;
	unsigned int dram_tpr12;
	unsigned int dram_tpr13;
	unsigned int dram_tpr14;
	unsigned int dram_tpr15;
	unsigned int dram_tpr16;
	unsigned int dram_tpr17;
	unsigned int dram_tpr18;
	unsigned int dram_tpr19;
	unsigned int dram_tpr20;
	unsigned int dram_tpr21;
};
//typedef struct dram_para_32 __dram_para_t;

struct rtos_img_params_t {
	/* 32 * 4 = 128 byte */
	struct dram_para_32 dram_para;
	/* size ~ 56Bytes + 64Bytes*2 + 28Bytes = 212 Bytes */
	/* struct audio_param audio_param; */
	/* unsigned int message_pool_phys; */
	/* unsigned int message_pool_size; */
	/* unsigned int standby_base; */
	/* unsigned int standby_size; */
	/* unsigned int suart_status; */
	/* unsigned int pmu_bat_shutdown_ltf; */
	/* unsigned int pmu_bat_shutdown_htf; */
	/* unsigned int pmu_pwroff_vol; */
	/* unsigned int power_mode; */
	/* unsigned int power_start; */
	/* unsigned int powchk_used; */
	/* unsigned int power_reg; */
	/* unsigned int system_power; */
	/* 7 * 4 = 28 byte */
	struct box_start_os_cfg start_os;
	/* struct ir_key ir_key; */
	/* unsigned int reserved[256]; */
};

struct spare_rtos_head_t {
	struct rtos_img_hdr_t	rtos_img_hdr;
	struct rtos_img_params_t	rtos_img_params;
	unsigned char padding[PADDING_LEN - sizeof(struct rtos_img_hdr_t) -
			      sizeof(struct rtos_img_params_t)];
	unsigned char cert_magic[MAGIC_SIZE];
	unsigned int cert_size;
	/* NOTICE: the 1k - 128 byete start version string */
};

#endif /* __SUNXI_PLATFORM_H */
