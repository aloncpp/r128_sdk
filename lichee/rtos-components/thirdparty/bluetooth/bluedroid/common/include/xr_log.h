// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __XR_LOG_H__
#define __XR_LOG_H__

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log level
 *
 */
typedef enum {
    XR_LOG_NONE,       /*!< No log output */
    XR_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    XR_LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
    XR_LOG_INFO,       /*!< Information messages which describe normal flow of events */
    XR_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    XR_LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} xr_log_level_t;

// typedef int (*vprintf_like_t)(const char *, va_list);

// void xr_log_level_set(const char* tag, xr_log_level_t level);

// vprintf_like_t xr_log_set_vprintf(vprintf_like_t func);

// char* xr_log_system_timestamp(void);

// uint32_t xr_log_early_timestamp(void);

#if CONFIG_LOG_TIMESTAMP
#include "kernel/os/os.h"
#define LOG_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%d) %s: " format LOG_RESET_COLOR "\n"
#define xr_log_timestamp() OS_TicksToMSecs(OS_GetTicks())
#define xr_log_write(level, tag1, format, time, tag2, ...) printf(format, time, tag2, ##__VA_ARGS__)
#else
#define LOG_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " %s: " format LOG_RESET_COLOR "\n"
#define xr_log_timestamp() (0)
#define xr_log_write(level, tag1, format, time, tag2, ...) printf(format, tag2, ##__VA_ARGS__)
#endif

// void xr_log_writev(xr_log_level_t level, const char* tag, const char* format, va_list args);

#ifndef LOG_LOCAL_LEVEL
#ifndef BOOTLOADER_BUILD
#define LOG_LOCAL_LEVEL  CONFIG_BLUEDRIOD_LOG_DEFAULT_LEVEL
#else
#define LOG_LOCAL_LEVEL  CONFIG_BOOTLOADER_LOG_LEVEL
#endif
#endif

void xr_log_buffer_hex_internal(const char *tag, const void *buffer, uint16_t buff_len,xr_log_level_t log_level);
void xr_log_buffer_char_internal(const char *tag, const void *buffer, uint16_t buff_len,xr_log_level_t log_level);
void xr_log_buffer_hexdump_internal(const char *tag, const void *buffer, uint16_t buff_len, xr_log_level_t log_level);

#if 1//(LOG_LOCAL_LEVEL > CONFIG_BLUEDRIOD_LOG_DEFAULT_LEVEL)
#define XR_LOG_BUFFER_HEX_LEVEL( tag, buffer, buff_len, level ) \
    do {\
        if ( LOG_LOCAL_LEVEL >= (level) ) { \
            xr_log_buffer_hex_internal( tag, buffer, buff_len, level ); \
        } \
    } while(0)

#define XR_LOG_BUFFER_CHAR_LEVEL( tag, buffer, buff_len, level ) \
    do {\
        if ( LOG_LOCAL_LEVEL >= (level) ) { \
            xr_log_buffer_char_internal( tag, buffer, buff_len, level ); \
        } \
    } while(0)

#define XR_LOG_BUFFER_HEXDUMP( tag, buffer, buff_len, level ) \
    do { \
        if ( LOG_LOCAL_LEVEL >= (level) ) { \
            xr_log_buffer_hexdump_internal( tag, buffer, buff_len, level); \
        } \
    } while(0)

#define XR_LOG_BUFFER_HEX(tag, buffer, buff_len) \
    do { \
        if (LOG_LOCAL_LEVEL >= XR_LOG_INFO) { \
            XR_LOG_BUFFER_HEX_LEVEL( tag, buffer, buff_len, XR_LOG_INFO ); \
        }\
    } while(0)

#define XR_LOG_BUFFER_CHAR(tag, buffer, buff_len) \
    do { \
        if (LOG_LOCAL_LEVEL >= XR_LOG_INFO) { \
            XR_LOG_BUFFER_CHAR_LEVEL( tag, buffer, buff_len, XR_LOG_INFO ); \
        }\
    } while(0)

//to be back compatible
#define xr_log_buffer_hex      XR_LOG_BUFFER_HEX
#define xr_log_buffer_char     XR_LOG_BUFFER_CHAR
#else
void print_hex_dump_bytes(const void *addr, unsigned int len);

#define xr_log_buffer_hex(tag, addr, len) {printf("D:" tag " ");print_hex_dump_bytes(addr, len);}
#define xr_log_buffer_char(tag, addr, len) printf("D:" tag " %*s", len, addr)
#endif/*#if (LOG_LOCAL_LEVEL > CONFIG_BLUEDRIOD_LOG_DEFAULT_LEVEL)*/

#if CONFIG_LOG_COLORS
#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31"
#define LOG_COLOR_GREEN   "32"
#define LOG_COLOR_BROWN   "33"
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_PURPLE  "35"
#define LOG_COLOR_CYAN    "36"
#define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define LOG_RESET_COLOR   "\033[0m"
#define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D
#define LOG_COLOR_V
#else //CONFIG_LOG_COLORS
#define LOG_COLOR_E
#define LOG_COLOR_W
#define LOG_COLOR_I
#define LOG_COLOR_D
#define LOG_COLOR_V
#define LOG_RESET_COLOR
#endif //CONFIG_LOG_COLORS

// #define LOG_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%u) %s: " format LOG_RESET_COLOR "\n"
#define LOG_SYSTEM_TIME_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%s) %s: " format LOG_RESET_COLOR "\n"

/** @endcond */

/// macro to output logs in startup code, before heap allocator and syscalls have been initialized. log at ``XR_LOG_ERROR`` level. @see ``printf``,``XR_LOGE``
#define XR_EARLY_LOGE( tag, format, ... ) XR_LOG_EARLY_IMPL(tag, format, XR_LOG_ERROR,   E, ##__VA_ARGS__)
/// macro to output logs in startup code at ``XR_LOG_WARN`` level.  @see ``XR_EARLY_LOGE``,``XR_LOGE``, ``printf``
#define XR_EARLY_LOGW( tag, format, ... ) XR_LOG_EARLY_IMPL(tag, format, XR_LOG_WARN,    W, ##__VA_ARGS__)
/// macro to output logs in startup code at ``XR_LOG_INFO`` level.  @see ``XR_EARLY_LOGE``,``XR_LOGE``, ``printf``
#define XR_EARLY_LOGI( tag, format, ... ) XR_LOG_EARLY_IMPL(tag, format, XR_LOG_INFO,    I, ##__VA_ARGS__)
/// macro to output logs in startup code at ``XR_LOG_DEBUG`` level.  @see ``XR_EARLY_LOGE``,``XR_LOGE``, ``printf``
#define XR_EARLY_LOGD( tag, format, ... ) XR_LOG_EARLY_IMPL(tag, format, XR_LOG_DEBUG,   D, ##__VA_ARGS__)
/// macro to output logs in startup code at ``XR_LOG_VERBOSE`` level.  @see ``XR_EARLY_LOGE``,``XR_LOGE``, ``printf``
#define XR_EARLY_LOGV( tag, format, ... ) XR_LOG_EARLY_IMPL(tag, format, XR_LOG_VERBOSE, V, ##__VA_ARGS__)

#define XR_LOG_EARLY_IMPL(tag, format, log_level, log_tag_letter, ...) do {                             \
        if (LOG_LOCAL_LEVEL >= log_level) {                                                              \
            xr_rom_printf(LOG_FORMAT(log_tag_letter, format), xr_log_timestamp(), tag, ##__VA_ARGS__); \
        }} while(0)

#ifndef BOOTLOADER_BUILD
#define XR_LOGE( tag, format, ... ) XR_LOG_LEVEL_LOCAL(XR_LOG_ERROR,   tag, format, ##__VA_ARGS__)
#define XR_LOGW( tag, format, ... ) XR_LOG_LEVEL_LOCAL(XR_LOG_WARN,    tag, format, ##__VA_ARGS__)
#define XR_LOGI( tag, format, ... ) XR_LOG_LEVEL_LOCAL(XR_LOG_INFO,    tag, format, ##__VA_ARGS__)
#define XR_LOGD( tag, format, ... ) XR_LOG_LEVEL_LOCAL(XR_LOG_DEBUG,   tag, format, ##__VA_ARGS__)
#define XR_LOGV( tag, format, ... ) XR_LOG_LEVEL_LOCAL(XR_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
#else
/**
 * macro to output logs at XR_LOG_ERROR level.
 *
 * @param tag tag of the log, which can be used to change the log level by ``xr_log_level_set`` at runtime.
 *
 * @see ``printf``
 */
#define XR_LOGE( tag, format, ... )  XR_EARLY_LOGE(tag, format, ##__VA_ARGS__)
/// macro to output logs at ``XR_LOG_WARN`` level.  @see ``XR_LOGE``
#define XR_LOGW( tag, format, ... )  XR_EARLY_LOGW(tag, format, ##__VA_ARGS__)
/// macro to output logs at ``XR_LOG_INFO`` level.  @see ``XR_LOGE``
#define XR_LOGI( tag, format, ... )  XR_EARLY_LOGI(tag, format, ##__VA_ARGS__)
/// macro to output logs at ``XR_LOG_DEBUG`` level.  @see ``XR_LOGE``
#define XR_LOGD( tag, format, ... )  XR_EARLY_LOGD(tag, format, ##__VA_ARGS__)
/// macro to output logs at ``XR_LOG_VERBOSE`` level.  @see ``XR_LOGE``
#define XR_LOGV( tag, format, ... )  XR_EARLY_LOGV(tag, format, ##__VA_ARGS__)
#endif  // BOOTLOADER_BUILD

/** runtime macro to output logs at a specified level.
 *
 * @param tag tag of the log, which can be used to change the log level by ``xr_log_level_set`` at runtime.
 * @param level level of the output log.
 * @param format format of the output log. see ``printf``
 * @param ... variables to be replaced into the log. see ``printf``
 *
 * @see ``printf``
 */
#if CONFIG_LOG_TIMESTAMP_SOURCE_RTOS
#define XR_LOG_LEVEL(level, tag, format, ...) do {                     \
        if (level==XR_LOG_ERROR )          { xr_log_write(XR_LOG_ERROR,      tag, LOG_FORMAT(E, format), xr_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==XR_LOG_WARN )      { xr_log_write(XR_LOG_WARN,       tag, LOG_FORMAT(W, format), xr_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==XR_LOG_DEBUG )     { xr_log_write(XR_LOG_DEBUG,      tag, LOG_FORMAT(D, format), xr_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==XR_LOG_VERBOSE )   { xr_log_write(XR_LOG_VERBOSE,    tag, LOG_FORMAT(V, format), xr_log_timestamp(), tag, ##__VA_ARGS__); } \
        else                                { xr_log_write(XR_LOG_INFO,       tag, LOG_FORMAT(I, format), xr_log_timestamp(), tag, ##__VA_ARGS__); } \
    } while(0)
#elif CONFIG_LOG_TIMESTAMP_SOURCE_SYSTEM
#define XR_LOG_LEVEL(level, tag, format, ...) do {                     \
        if (level==XR_LOG_ERROR )          { xr_log_write(XR_LOG_ERROR,      tag, LOG_SYSTEM_TIME_FORMAT(E, format), xr_log_system_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==XR_LOG_WARN )      { xr_log_write(XR_LOG_WARN,       tag, LOG_SYSTEM_TIME_FORMAT(W, format), xr_log_system_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==XR_LOG_DEBUG )     { xr_log_write(XR_LOG_DEBUG,      tag, LOG_SYSTEM_TIME_FORMAT(D, format), xr_log_system_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==XR_LOG_VERBOSE )   { xr_log_write(XR_LOG_VERBOSE,    tag, LOG_SYSTEM_TIME_FORMAT(V, format), xr_log_system_timestamp(), tag, ##__VA_ARGS__); } \
        else                                { xr_log_write(XR_LOG_INFO,       tag, LOG_SYSTEM_TIME_FORMAT(I, format), xr_log_system_timestamp(), tag, ##__VA_ARGS__); } \
    } while(0)
#endif //CONFIG_LOG_TIMESTAMP_SOURCE_xxx

#ifndef XR_LOG_LEVEL
#define XR_LOG_LEVEL(level, tag, format, ...) do {                     \
        if (level==XR_LOG_ERROR )          { xr_log_write(XR_LOG_ERROR,      tag, LOG_FORMAT(E, format), xr_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==XR_LOG_WARN )      { xr_log_write(XR_LOG_WARN,       tag, LOG_FORMAT(W, format), xr_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==XR_LOG_DEBUG )     { xr_log_write(XR_LOG_DEBUG,      tag, LOG_FORMAT(D, format), xr_log_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level==XR_LOG_VERBOSE )   { xr_log_write(XR_LOG_VERBOSE,    tag, LOG_FORMAT(V, format), xr_log_timestamp(), tag, ##__VA_ARGS__); } \
        else                                { xr_log_write(XR_LOG_INFO,       tag, LOG_FORMAT(I, format), xr_log_timestamp(), tag, ##__VA_ARGS__); } \
    } while(0)
#endif

/** runtime macro to output logs at a specified level. Also check the level with ``LOG_LOCAL_LEVEL``.
 *
 * @see ``printf``, ``XR_LOG_LEVEL``
 */
#define XR_LOG_LEVEL_LOCAL(level, tag, format, ...) do {               \
        if ( LOG_LOCAL_LEVEL >= level ) XR_LOG_LEVEL(level, tag, format, ##__VA_ARGS__); \
    } while(0)


/**
 * @brief Macro to output logs when the cache is disabled. log at ``XR_LOG_ERROR`` level.
 *
 * Similar to `XR_EARLY_LOGE`, the log level cannot be changed by `xr_log_level_set`.
 *
 * Usage: `XR_DRAM_LOGE(DRAM_STR("my_tag"), "format", or `XR_DRAM_LOGE(TAG, "format", ...)`,
 * where TAG is a char* that points to a str in the DRAM.
 *
 * @note Placing log strings in DRAM reduces available DRAM, so only use when absolutely essential.
 *
 * @see ``xr_rom_printf``,``XR_LOGE``
 */
#define XR_DRAM_LOGE( tag, format, ... ) XR_DRAM_LOG_IMPL(tag, format, XR_LOG_ERROR,   E, ##__VA_ARGS__)
/// macro to output logs when the cache is disabled at ``XR_LOG_WARN`` level.  @see ``XR_DRAM_LOGW``,``XR_LOGW``, ``xr_rom_printf``
#define XR_DRAM_LOGW( tag, format, ... ) XR_DRAM_LOG_IMPL(tag, format, XR_LOG_WARN,    W, ##__VA_ARGS__)
/// macro to output logs when the cache is disabled at ``XR_LOG_INFO`` level.  @see ``XR_DRAM_LOGI``,``XR_LOGI``, ``xr_rom_printf``
#define XR_DRAM_LOGI( tag, format, ... ) XR_DRAM_LOG_IMPL(tag, format, XR_LOG_INFO,    I, ##__VA_ARGS__)
/// macro to output logs when the cache is disabled at ``XR_LOG_DEBUG`` level.  @see ``XR_DRAM_LOGD``,``XR_LOGD``, ``xr_rom_printf``
#define XR_DRAM_LOGD( tag, format, ... ) XR_DRAM_LOG_IMPL(tag, format, XR_LOG_DEBUG,   D, ##__VA_ARGS__)
/// macro to output logs when the cache is disabled at ``XR_LOG_VERBOSE`` level.  @see ``XR_DRAM_LOGV``,``XR_LOGV``, ``xr_rom_printf``
#define XR_DRAM_LOGV( tag, format, ... ) XR_DRAM_LOG_IMPL(tag, format, XR_LOG_VERBOSE, V, ##__VA_ARGS__)

/** @cond */
#define _XR_LOG_DRAM_LOG_FORMAT(letter, format)  DRAM_STR(#letter " %s: " format "\n")

#define XR_DRAM_LOG_IMPL(tag, format, log_level, log_tag_letter, ...) do {                       \
        if (LOG_LOCAL_LEVEL >= log_level) {                                                       \
            xr_rom_printf(_XR_LOG_DRAM_LOG_FORMAT(log_tag_letter, format), tag, ##__VA_ARGS__); \
        }} while(0)
/** @endcond */

#ifdef __cplusplus
}
#endif


#endif /* __XR_LOG_H__ */
