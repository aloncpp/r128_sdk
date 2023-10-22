#include <stdio.h>
#include <ctype.h>
#include "console.h"
#include <FreeRTOS.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <task.h>
#include <getopt.h>
#include <io.h>
#include <hal_mem.h>
#include <hal_cache.h>
#include <sunxi_hal_ce.h>

//#include <portmacro.h>
//#include <cmsis_compiler.h>
//#include <cachel1_armv7.h>
#include "tfm_sunxi_nsc.h"

#ifndef configAPPLICATION_NORMAL_PRIORITY
#define configAPPLICATION_NORMAL_PRIORITY (15)
#endif

static uint32_t ulNonSecureCounter __attribute__((aligned(32))) = 0;
static uint32_t ulCurrentSecureCounter __attribute__((aligned(32))) = 0;

/**
 * @brief Increments the ulNonSecureCounter.
 *
 * This function is called from the secure side.
 */
static void prvCallback( void )
{
	/* This function is called from the secure side. Just increment the counter
	 * here. The check that this counter keeps incrementing is performed in the
	 * prvSecureCallingTask. */
	ulNonSecureCounter ++;
}

static void prvSecureCallingTask( void * pvParameters )
{
	uint32_t ulLastSecureCounter = ulCurrentSecureCounter;
	uint32_t ulLastNonSecureCounter = ulNonSecureCounter;
	uint32_t i = 0;

	while(i < 10)
	{
		/* Call the secure side function. It does two things:
		 * - It calls the supplied function (prvCallback) which in turn
		 *   increments the non-secure counter.
		 * - It increments the secure counter and returns the incremented value.
		 * Therefore at the end of this function call both the secure and
		 * non-secure counters must have been incremented.
		*/
		ulCurrentSecureCounter = tfm_sunxi_nsc_func(prvCallback);

		/* Make sure that both the counters are incremented. */
		configASSERT(ulCurrentSecureCounter == ulLastSecureCounter + 1);
		configASSERT(ulNonSecureCounter == ulLastNonSecureCounter + 1);

		/* Update the last values for both the counters. */
		ulLastSecureCounter = ulCurrentSecureCounter;
		ulLastNonSecureCounter = ulNonSecureCounter;

		printf("secure call %d time, non-secure call %d time. \n", ulLastSecureCounter, ulLastNonSecureCounter);

		/* Wait for a second. */
		vTaskDelay(pdMS_TO_TICKS(1000));

		i++;
	}

	vTaskDelete(NULL);
}

int cmd_tfm_demo(int argc, char ** argv)
{
	TaskHandle_t tfm_task;
	portBASE_TYPE ret;

	ret = xTaskCreate(prvSecureCallingTask, (signed portCHAR *) "tfm demo", 4096, NULL, configAPPLICATION_NORMAL_PRIORITY, &tfm_task);
	if (ret != pdPASS)
	{
		printf("Error creating task, status was %d\n", ret);
		return -1;
	}
}
FINSH_FUNCTION_EXPORT_CMD(cmd_tfm_demo, tfm_demo, tfm ns/s switch test);

static int hexstr_to_byte(const char* source, uint8_t* dest, int sourceLen)
{
	uint32_t i;
	uint8_t highByte, lowByte;

	for (i = 0; i < sourceLen; i += 2) {
		highByte = toupper(source[i]);
		lowByte  = toupper(source[i + 1]);

		if (highByte < '0' || (highByte > '9' && highByte < 'A' ) || highByte > 'F') {
			printf("input buf[%d] is %c, not in 0123456789ABCDEF\n", i, source[i]);
			return -1;
		}

		if (lowByte < '0' || (lowByte > '9' && lowByte < 'A' ) || lowByte > 'F') {
			printf("input buf[%d] is %c, not in 0123456789ABCDEF\n", i+1, source[i+1]);
			return -1;
		}

		if (highByte > 0x39)
			highByte -= 0x37;
		else
			highByte -= 0x30;


		if (lowByte > 0x39)
			lowByte -= 0x37;
		else
			lowByte -= 0x30;

		dest[i / 2] = (highByte << 4) | lowByte;
	}
	return 0;
}

int cmd_tfm_efuse_write(int argc, char ** argv)
{
        char *key_name = NULL;
        char *key_hex_string = NULL;
        uint32_t buf_len = 0;
        char *key_data = NULL;
        int ret = 0;

        if (argc != 3) {
                printf("Argument Error!\n");
                printf("Usage: tfm_efuse_write <key_name> <key_hex_string>\n");
                return -1;
        }

        key_name = argv[1];
        key_hex_string = argv[2];
        buf_len = strlen(argv[2]);
        printf("key_hex_string: %s, buf_len: %d\n", key_hex_string, buf_len);
        if ( buf_len % 2 != 0) {
                printf("key_hex_string len: %d is error!\n", buf_len);
		return -1;
        }

        key_data = (char *)hal_malloc_align(buf_len / 2, CACHELINE_LEN);
        if (!key_data) {
                printf("malloc %d bytes error!", buf_len / 2);
                return -1;
        }

        hexstr_to_byte(key_hex_string, key_data, buf_len);

	hal_dcache_clean(key_data, buf_len / 2);

        ret = tfm_sunxi_efuse_write(key_name, key_data, (buf_len / 2) << 3);
        if (ret)
                printf("efuse write error: %d\n", ret);

        hal_free_align(key_data);
        return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_tfm_efuse_write, tfm_efuse_write, tfm efuse write);


#define AES_MODE_ECB		(0)
#define AES_DIR_ENCRYPT		(0)
#define AES_DIR_DECRYPT		(1)
int cmd_tfm_crypto_demo(int argc, char ** argv)
{
	int ret = 0;
	crypto_aes_req_ctx_t *aes_ctx = NULL;
	__aligned(64) uint8_t aes_src_16[16] = {
		0x43, 0xdd, 0xe8, 0x30, 0x06, 0xf4, 0x1e, 0x25, 0xf9, 0x27, 0x4b, 0x3c, 0x67, 0xe9, 0x3b, 0x06,
	};
	__aligned(64) uint8_t aes_key_128[16] = {
		0x6a, 0x51, 0x8a, 0xf3, 0x3e, 0x16, 0xb5, 0xc4, 0x4e, 0x0f, 0x87, 0xc7, 0x55, 0x31, 0xc3, 0x2b,
	};
	__aligned(64) uint8_t aes_128_ecb_16[16] = {
		0x7a, 0xff, 0x00, 0xfc, 0x79, 0xa7, 0xad, 0x62, 0xb2, 0xdd, 0x80, 0x4b, 0x28, 0xc7, 0x0d, 0xe8,
	};
	aes_ctx = (crypto_aes_req_ctx_t *)hal_malloc_align(sizeof(crypto_aes_req_ctx_t), max(CE_ALIGN_SIZE, CACHELINE_LEN));
	if (aes_ctx == NULL) {
		printf (" malloc data buffer fail\n");
		return -1;
	}
	memset(aes_ctx, 0x0, sizeof(crypto_aes_req_ctx_t));

	aes_ctx->dst_buffer = (u8 *)hal_malloc_align(16, max(CE_ALIGN_SIZE, CACHELINE_LEN));
	if (aes_ctx->dst_buffer == NULL) {
		printf (" malloc dest buffer fail\n");
		ret = -1;
		goto out;
	}
	memset(aes_ctx->dst_buffer, 0, 16);
	aes_ctx->src_buffer = aes_src_16;
	aes_ctx->src_length = 16;
	aes_ctx->dst_length = 16;
	aes_ctx->key = aes_key_128;
	aes_ctx->key_length = 16;
	aes_ctx->iv = NULL;
	aes_ctx->mode = AES_MODE_ECB;
	aes_ctx->dir = AES_DIR_ENCRYPT;

	hal_dcache_clean((unsigned long)aes_ctx, sizeof(crypto_aes_req_ctx_t));
	hal_dcache_clean((unsigned long)aes_ctx->dst_buffer, aes_ctx->dst_length);
	hal_dcache_clean((unsigned long)aes_ctx->src_buffer, aes_ctx->src_length);
	hal_dcache_clean((unsigned long)aes_ctx->key, aes_ctx->key_length);
	ret = tfm_sunxi_aes_with_hardware(aes_ctx);
	hal_dcache_invalidate((unsigned long)aes_ctx->dst_buffer, aes_ctx->dst_length);

	if (ret) {
		printf("tfm_sunxi_aes_with_hardware enc error: %d\n", ret);
		goto out;
	}

	if (memcmp(aes_ctx->dst_buffer, aes_128_ecb_16, 16) != 0) {
		printf("aes ecb enc error, calc data:\n");
		hexdump(aes_ctx->dst_buffer, 16);
		printf("want data:\n");
		hexdump(aes_128_ecb_16, 16);
		ret = -1;
		goto out;
	}

	memset(aes_ctx->dst_buffer, 0x0, aes_ctx->dst_length);
	aes_ctx->dir = AES_DIR_DECRYPT;
	aes_ctx->src_buffer = aes_128_ecb_16;
	aes_ctx->src_length = 16;

	hal_dcache_clean((unsigned long)aes_ctx, sizeof(crypto_aes_req_ctx_t));
	hal_dcache_clean((unsigned long)aes_ctx->dst_buffer, aes_ctx->dst_length);
	hal_dcache_clean((unsigned long)aes_ctx->src_buffer, aes_ctx->src_length);
	ret = tfm_sunxi_aes_with_hardware(aes_ctx);
	hal_dcache_invalidate((unsigned long)aes_ctx->dst_buffer, aes_ctx->dst_length);

	if (ret) {
		printf("tfm_sunxi_aes_with_hardware dec error: %d\n", ret);
		goto out;
	}

	if (memcmp(aes_ctx->dst_buffer, aes_src_16, 16) != 0) {
		printf("aes ecb dec error, calc data:\n");
		hexdump(aes_ctx->dst_buffer, 16);
		printf("want data:\n");
		hexdump(aes_src_16, 16);
		ret = -1;
		goto out;
	}

	printf("tfm aes ecb test success!\n");
out:
	if (aes_ctx->dst_buffer != NULL) {
		hal_free_align(aes_ctx->dst_buffer);
	}

	hal_free_align(aes_ctx);

	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_tfm_crypto_demo, tfm_crypto_demo, tfm ecb test);
