#ifndef __SUNXI_INPUT_H
#define __SUNXI_INPUT_H

#include <hal_osal.h>
#include "semphr.h"
#include "aw_list.h"
#include "input-event-codes.h"

/*********************************bit define******************************************/
#define INPUT_BITS_PER_BYTE 8
#define INPUT_DIV_ROUND_UP(n,d)		(((n) + (d) - 1) / (d))
#define INPUT_BITS_TO_LONGS(nr)		INPUT_DIV_ROUND_UP(nr, INPUT_BITS_PER_BYTE * sizeof(long))


static inline void input_set_bit(int nr,  volatile void * addr)
{
	((int *) addr)[nr >> 5] |= (1UL << (nr & 31));
}

static inline int input_test_bit(int nr, const volatile void * addr)
{
	return (1UL & (((const int *) addr)[nr >> 5] >> (nr & 31))) != 0UL;
}



/******************************input event define***************************************/
#define EVENT_BUFFER_SIZE 64U

struct sunxi_input_event {
	unsigned int type;
	unsigned int code;
	unsigned int value;
};

struct sunxi_input_dev {
	const char *name;

	unsigned long evbit[INPUT_BITS_TO_LONGS(EV_CNT)];
	unsigned long keybit[INPUT_BITS_TO_LONGS(KEY_CNT)];
	unsigned long relbit[INPUT_BITS_TO_LONGS(REL_CNT)];
	unsigned long absbit[INPUT_BITS_TO_LONGS(ABS_CNT)];
	unsigned long mscbit[INPUT_BITS_TO_LONGS(MSC_CNT)];

	struct list_head        h_list;
	struct list_head        node;

};

//one task that read input dev, corresponds one sunxi_evdev
struct sunxi_evdev {
	int fd;
	const char *name;
	hal_sem_t sem;

	struct sunxi_input_event buffer[EVENT_BUFFER_SIZE];
	unsigned int head;
	unsigned int tail;
	unsigned int packet_head;

	struct list_head        h_list;
	struct list_head        node;

};

void input_set_capability(struct sunxi_input_dev *dev, unsigned int type, unsigned int code);
struct sunxi_input_dev *sunxi_input_allocate_device(void);
int sunxi_input_register_device(struct sunxi_input_dev *dev);
void sunxi_input_event(struct sunxi_input_dev *dev,
			unsigned int type, unsigned int code, unsigned int value);
int sunxi_input_read(int fd, void *buffer, unsigned int size);
int sunxi_input_readb(int fd, void *buffer, unsigned int size);
int sunxi_input_open(const char *name);
static inline void input_report_key(struct sunxi_input_dev *dev, unsigned int code, int value)
{
	sunxi_input_event(dev, EV_KEY, code, !!value);
}

static inline void input_report_rel(struct sunxi_input_dev *dev, unsigned int code, int value)
{
	sunxi_input_event(dev, EV_REL, code, value);
}

static inline void input_report_abs(struct sunxi_input_dev *dev, unsigned int code, int value)
{
	sunxi_input_event(dev, EV_ABS, code, value);
}
static inline void input_report_misc(struct sunxi_input_dev *dev, unsigned int code, int value)
{
	sunxi_input_event(dev, EV_MSC, code, value);
}
static inline void input_sync(struct sunxi_input_dev *dev)
{
	sunxi_input_event(dev, EV_SYN, SYN_REPORT, 0);
}

#endif//__SUNXI_INPUT_H
