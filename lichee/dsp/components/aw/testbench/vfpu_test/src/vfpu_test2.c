#ifdef CONFIG_ARCH_DSP
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>

#include <FreeRTOS.h>
#include <task.h>

#include <console.h>
#include <hal_mem.h>
#include <hal_time.h>

#include "NatureDSP_Signal.h"

#if 1
__attribute__((weak)) void *hal_malloc_align(uint32_t size, int align)
{
	void *ptr;
	void *align_ptr;
	int uintptr_size;
	int align_size;

	/* sizeof pointer */
	uintptr_size = sizeof(void*);
	uintptr_size -= 1;

	/* align the alignment size to uintptr size byte */
	align = ((align + uintptr_size) & ~uintptr_size);

	/* get total aligned size */
	align_size = ((size + uintptr_size) & ~uintptr_size) + align;
	/* allocate memory block from heap */
	ptr = hal_malloc(align_size);
	if (ptr != NULL)
	{
		/* the allocated memory block is aligned */
		if (((uint32_t)ptr & (align - 1)) == 0)
		{
			align_ptr = (void *)((uint32_t)ptr + align);
		}
		else
		{
			align_ptr = (void *)(((uint32_t)ptr + (align - 1)) & ~(align - 1));
		}

		/* set the pointer before alignment pointer to the real pointer */
		*((uint32_t *)((uint32_t)align_ptr - sizeof(void *))) = (uint32_t)ptr;

		ptr = align_ptr;
	}

	return ptr;
}

__attribute__((weak)) void hal_free_align(void *p)
{
	void *real_ptr;

	real_ptr = (void *) * (uint32_t *)((uint32_t)p - sizeof(void *));
	hal_free(real_ptr);
}
#endif

static const char *state_buf[4] = {"\r-", "\r\\", "\r|", "\r/"};
static inline void update_state(unsigned int *state)
{
	printf("%s", state_buf[(*state)%4]);
	fflush(stdout);
	(*state)++;
}

struct cplxf_t {
	float r;
	float i;
};

#ifndef PI
#define PI 			(3.141592653f)
#endif

static inline void show_array_cplxf(struct cplxf_t *array, int N)
{
	int i = 0;
	for (i = 0; i < N; i++) {
		float val = array[i].r * array[i].r + array[i].i * array[i].i;
		val = sqrt(val);
		printf("[%u] %f\t(%f + %fj)\n", i, val, array[i].r, array[i].i);
	}
}

static inline void make_src_cplxf(struct cplxf_t *output, int N, int off, float freq, float max)
{
	int i = 0;
	for (i = 0; i < N; i++) {
		output[i].r = (sinf(2 * PI * ((i + off) % (int)freq) / freq) * max);
		output[i].i = 0.0f;
	}
}

//N*3/4 *twdstep
static inline void make_twd_cplxf(struct cplxf_t *output, const int N, const int twdstep)
{
	int n, m;
	for (n = 0; n < (twdstep * N) / 4; n++) {
		for (m = 0; m < 3; m++) {
            float phi = 2 * PI * (m + 1) * n / (twdstep * N);
			output[n * 3 + m].r = cosf(phi);
			output[n * 3 + m].i = sinf(phi);
        }
    }
}

static volatile int g_run_flag;
static unsigned int g_data_length;
static unsigned int g_sub_length;
static int g_loop_times;
static unsigned int g_thread_num;
static unsigned int g_verbose;
static void vfpu_test_thread(void *arg)
{
	uint64_t start = 0, end = 0;
	unsigned int data_length = g_data_length;
	unsigned int sub_length = g_sub_length;
	unsigned int sub_times = data_length / sub_length;
	int loop_times = g_loop_times;
	int cur_loop_times = 0;
	int times_per_update = g_loop_times;
	int verbose = g_verbose;
	unsigned int j;

	struct cplxf_t *output_cplxf = NULL;
	struct cplxf_t *input_cplxf = NULL;
	struct cplxf_t *src_cplxf = NULL;
	struct cplxf_t *twd_cplxf = NULL;

	unsigned int state = 0;
	unsigned int output_num = data_length;
	unsigned int input_num = sub_length;
	unsigned int src_num = sub_length;
	int twdstep = 1;
	unsigned int twd_num = sub_length * twdstep * 3 / 4;

	printf("%s enter\n", pcTaskGetName(NULL));

	output_cplxf = hal_malloc_align(sizeof(struct cplxf_t) * (output_num + input_num + src_num + twd_num), 64);
	if (!output_cplxf) {
		printf("no memory!\n");
		goto exit;
	}
	printf("memory requst:  %uKB\n", sizeof(struct cplxf_t) * (output_num + input_num + src_num + twd_num) / 1024);

	input_cplxf = &output_cplxf[output_num];
	src_cplxf = &input_cplxf[input_num];
	twd_cplxf = &src_cplxf[src_num];

	printf("%s output: %08lx, input: %08lx, src: %08lx, twdstep: %d, twd: %08lx\n",
		pcTaskGetName(NULL),
		(unsigned long)output_cplxf,
		(unsigned long)input_cplxf,
		(unsigned long)src_cplxf,
		twdstep,
		(unsigned long)twd_cplxf);

	times_per_update = 8192 * 10000 / data_length;

	make_src_cplxf(src_cplxf, src_num, 0, 32.0f, 100.0f);
	make_twd_cplxf(twd_cplxf, sub_length, twdstep);
	hal_msleep(1000);

	printf("%s start\n", pcTaskGetName(NULL));
	start = xTaskGetTickCount();
	while (g_run_flag) {
		for(j = 0; j < sub_times; j++){
			memcpy(input_cplxf, src_cplxf, input_num * sizeof(*input_cplxf));
			fft_cplxf_ie(
				(complex_float *)&output_cplxf[j * sub_length],
				(complex_float *)input_cplxf,
				(const complex_float *)twd_cplxf,
				1,
				sub_length);
		}
		cur_loop_times++;
		if (loop_times > 0 && cur_loop_times >= loop_times)
			break;
		if (g_thread_num == 1 && !(cur_loop_times % times_per_update))
			update_state(&state);
	}
	end = xTaskGetTickCount();
exit:
	if (output_cplxf) {
		if (verbose == 1) {
			show_array_cplxf(output_cplxf, sub_length);
		}
		hal_free_align(output_cplxf);
	}

	printf("%s start tick: %llu, end tick: %llu , portTICK_PERIOD_MS: %u\n", pcTaskGetName(NULL), (uint64_t)start, (uint64_t)end, (uint32_t)portTICK_PERIOD_MS);
	printf("%s exit, total time cost: %fs\n", pcTaskGetName(NULL), (double)(end - start) * portTICK_PERIOD_MS / 1000);
	vTaskDelete(NULL);
}

static void print_help(void)
{
	printf("usage:\n");
	printf("\t vfpu_test1 -t [thread_num] -l [loop_times] -d [data_length] -s [sub_length]\n");
	printf("\t\t -t [thread_num]:  default is 1\n");
	printf("\t\t -t [loop_times]:  default is -1, never stop until run vfpu_test1_stop\n");
	printf("\t\t -d [data_length]: default is 262144, align to sub_length\n");
	printf("\t\t -d [sub_length]:  default is 1024, align to 8\n");
	printf("\t\t note: memory requst: [thread_num] * [data_length] * 8\n");
}

static int get_param(int argc, char **argv)
{
	unsigned int data_length = 0;
	unsigned int sub_length = 0;
	int loop_times = 0;
	unsigned int thread_num = 0;
	unsigned int verbose = 0;
	int ch = 0;

	optarg = NULL;
	optind = opterr = optopt = 0;

	g_data_length = 16 * 1024;
	g_sub_length = 1024;
	g_loop_times = -1;
	g_thread_num = 1;
	g_verbose = 0;

	for (;;) {
		ch = getopt(argc, argv, "t:l:s:d:v:h");
		if (ch < 0)
			break;
		switch (ch) {
		case 't':
			thread_num = atoi(optarg);
			break;
		case 'l':
			loop_times = atoi(optarg);
			break;
		case 'd':
			data_length = atoi(optarg);
			break;
		case 's':
			sub_length = atoi(optarg);
			break;
		case 'v':
			verbose = atoi(optarg);
			break;
		case 'h':
			goto err;
		}
	}

	if (thread_num > 0)
		g_thread_num = thread_num;
	else if (thread_num < 0)
		goto err;
	if (sub_length > 0 && (sub_length % 8) == 0)
		g_sub_length = sub_length;
	else if (sub_length < 0)
		goto err;
	if (data_length > 0 && (sub_length % g_sub_length) == 0)
		g_data_length = data_length;
	else if (data_length < 0)
		goto err;
	if (loop_times)
		g_loop_times = loop_times;
	if (verbose)
		g_verbose = verbose;

	printf("thread_num:  %u\n", g_thread_num);
	printf("data_length: %u\n", g_data_length);
	printf("sub_length:  %u\n", g_sub_length);
	printf("loop_times:  %d\n", g_loop_times);

	return 0;
err:
	print_help();
	return -1;
}

int cmd_vfpu_test2(int argc, char *argv[])
{
	unsigned int i;

	if (get_param(argc, argv)) {
		return -1;
	}

	printf("start, run vfpu_test1_stop to stop\n");
	g_run_flag = 1;

	for (i = 0; i < g_thread_num; i++) {
		char name[16];
		memset(name, 0, 16);
		snprintf(name, 16, "vfpu_test_t%u", i);
		if(xTaskCreate(vfpu_test_thread, name, 0x2e00 / sizeof( StackType_t ), NULL, 2 + i%3, NULL) != pdPASS) {
			printf("xTaskCreate %s failed!\n", name);
		}
	}
	printf("done\n");
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_vfpu_test2, vfpu_test2, vfpu test2);

int cmd_vfpu_test2_stop(int argc, char *argv[]) {
	printf("stop\n");
	g_run_flag = 0;
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_vfpu_test2_stop, vfpu_test2_stop, stop vfpu test2);
#endif
