#ifndef __CACHE_H__
#define __CACHE_H__

#include <stddef.h>
#include <stdint.h>

void FlushDcacheAll(void);
void InvalidIcacheAll(void);
void FlushCacheAll(void);
void FlushIcacheAll(void);
void InvalidIcacheRegion(unsigned long start, unsigned int size);
void FlushDcacheRegion(unsigned long start, unsigned int size);
void InvalidDcacheRegion(unsigned long start, unsigned int size);
void InvalidIcacheRegion(unsigned long start, unsigned int size);
void InvalidDcache(void);
void FlushTLB(void);
void flush_dcache(void);
void init_mmu(void);
void init_cache(void);
void InitNeonVFP(void);

int xport_is_valid_address(void* start_addr, void* end_addr);

#endif //#ifndef __CACHE_H__
