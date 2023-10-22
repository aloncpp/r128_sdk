#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <generated/version.h>

#define __section_t(S)          __attribute__((__section__(#S)))
#define __version              __section_t(.version_table)

const char __version str_ver[] = SUB_VER "-" __DATE__ __TIME__;
