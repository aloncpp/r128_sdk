#ifndef _MMU_CACHE_H
#define _MMU_CACHE_H

void FlushDcacheAll(void);
void FlushIcacheAll(void);
void FlushCacheAll(void);
void FlushIcacheRegion(unsigned long start, unsigned int size);
void FlushDcacheRegion(unsigned long start, unsigned int size);
void InvalidDcacheRegion(unsigned long start, unsigned int size);
void InvalidIcacheRegion(unsigned long start, unsigned int size);
void InvalidDcache(void);

#endif
