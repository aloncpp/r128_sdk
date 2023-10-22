#include <xtensa/hal.h>

#include <FreeRTOS.h>
#include <task.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_MEM_SIZE	32

/*
 * return    0 - success
 *          -1 - failed
 */
static int cache_test(void)
{
	int i;
	char *src = NULL, *dst = NULL;
	char *s_cache = NULL, *d_cache = NULL;

	printf("Cache test begins...\n\n");

	src = malloc(TEST_MEM_SIZE);
	if (src == NULL) {
		printf("malloc src error!\n");
		return -1;
	}

	dst = malloc(TEST_MEM_SIZE);
	if (dst == NULL) {
		printf("malloc dst error!\n");
		free(src);
		return -1;
	}


	printf("1. Allocate source and destination memory from [Non-Cache region]:\n"
		"src addr:%p, dst addr:%p\n\n", src, dst);

	s_cache = src + 0x20000000;
	d_cache = dst + 0x20000000;

	printf("2. Map the memory to the [Cache region]:\n"
		"s_cache addr:%p, d_cache addr:%p\n\n", s_cache, d_cache);

	memset(src, 0, TEST_MEM_SIZE);
	memset(dst, 0, TEST_MEM_SIZE);

	printf("3. Initialize all source memory [Cache region] with 'a' -- OK.\n\n");

	for (i = 0; i < TEST_MEM_SIZE - 1; i++)
		s_cache[i] = 'a';
	s_cache[TEST_MEM_SIZE - 1] = '\0';

	printf("4. Before flush Dcache, all source memory [in RAM] should be NULL.\n"
		"src: %s\n\n", src);

	/* s_cache: flush Dcache */
	xthal_dcache_region_writeback(s_cache, TEST_MEM_SIZE);

	printf("5. After flush Dcache, all source memory [in RAM] should be updated to 'a'.\n"
		"src: %s\n\n", src);

	for (i = 0; i < TEST_MEM_SIZE - 1; i++)
		src[i] = 'b';
	src[TEST_MEM_SIZE - 1] = '\0';

	printf("6. Modifiy all source memory [in RAM] with 'b' -- OK.\n\n");

	printf("7. Before invalidate Dcache, all source memory [Cache region] should remain to be 'a'.\n"
		"s_cache: %s\n\n", s_cache);

	/* s_cache: invalidate Dcache */
	xthal_dcache_region_invalidate(s_cache, TEST_MEM_SIZE);

	printf("8. After invalidate Dcache, all source memory [Cache region] should be updated to 'b'.\n"
		"s_cache: %s\n\n", s_cache);

	free(src);
	free(dst);

	printf("Cache test finishes!\n\n");

	return 0;
}

static void mem_test_task(void *pdata)
{
	if (!cache_test())
		printf("Cache test success!\n");
	else
		printf("Cache test failed!\n");

	vTaskDelete(NULL);
}

void mem_test(void)
{
	uint32_t err = 0;

	err = xTaskCreate(mem_test_task, "mem_test", 0x1000, NULL, 1, NULL);
	if (err != pdPASS) {
		printf("Create memory test task failed!\n");
		return;
	}
}
