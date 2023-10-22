#include <stdio.h>
#include <stdlib.h>
#include <platform_utils.h>
uint32_t platform_random(uint32_t max)
{
	unsigned int num;
	srand(time(0));
	num = rand()%(max - 1) + 1;
	return num;
}
