
#include <xtensa/hal.h>

#if defined (__cplusplus)
extern "C"
#endif
const struct xthal_MPU_entry
__xt_mpu_init_table[] __attribute__((section(".ResetHandler.text"))) = {
  XTHAL_MPU_ENTRY(0x00000000, 1, XTHAL_AR_NONE, XTHAL_MEM_DEVICE), // unused
  XTHAL_MPU_ENTRY(0x0c000000, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_WRITEBACK), // extra_mem
  XTHAL_MPU_ENTRY(0x0c800000, 1, XTHAL_AR_NONE, XTHAL_MEM_DEVICE), // unused
};

#if defined (__cplusplus)
extern "C"
#endif
const unsigned int
__xt_mpu_init_table_size __attribute__((section(".ResetHandler.text"))) = 3;

#if defined (__cplusplus)
extern "C"
#endif
const unsigned int
__xt_mpu_init_cacheadrdis __attribute__((section(".ResetHandler.text"))) = 254;


#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#include <assert.h>
static_assert(sizeof(__xt_mpu_init_table)/sizeof(__xt_mpu_init_table[0]) == 3, "Incorrect MPU table size");
static_assert(sizeof(__xt_mpu_init_table)/sizeof(__xt_mpu_init_table[0]) <= XCHAL_MPU_ENTRIES, "MPU table too large");
#endif

