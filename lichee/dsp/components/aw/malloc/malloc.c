/* malloc function */
#include "xalloc.h"
#include "yfuns.h"

#include <FreeRTOS.h>
#include <task.h>

void *pvPortRealloc(void * old, size_t newlen);
void *pvPortCalloc(size_t count, size_t size);

_C_LIB_DECL

void *(malloc)(size_t size_arg)
{
	return pvPortMalloc(size_arg);
}

void (free)(void *ptr)
{
	vPortFree(ptr);
}

void *(realloc)(void *ptr, size_t size_arg)
{
	return pvPortRealloc(ptr, size_arg);
}

void *(calloc)(size_t nelem, size_t size)
{
	return pvPortCalloc(nelem, size);
}

_END_C_LIB_DECL

