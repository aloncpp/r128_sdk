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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "osi/allocator.h"
#include "osi/config.h"
#include "osi/list.h"
#include "common/bt_trace.h"

#if CONFIG_BLUEDROID_USE_SETTINGS
#include "settings/settings.h"
#endif

#define LOG_TAG "bt_osi_config"

typedef struct {
    char *key;
    char *value;
} entry_t;

typedef struct {
    char *name;
    list_t *entries;
} section_t;

struct config_t {
    list_t *sections;
};

#if CONFIG_BLUEDROID_USE_SETTINGS
typedef struct {
    int len;
    char *buf;
}cfg_t;
#endif

// Empty definition; this type is aliased to list_node_t.
struct config_section_iter_t {};

static void config_parse(config_t *config);

static section_t *section_new(const char *name);
static void section_free(void *ptr);
static section_t *section_find(const config_t *config, const char *section);

static entry_t *entry_new(const char *key, const char *value);
static void entry_free(void *ptr);
static entry_t *entry_find(const config_t *config, const char *section, const char *key);

config_t *config_new_empty(void)
{
    config_t *config = osi_calloc(sizeof(config_t));
    if (!config) {
        OSI_TRACE_ERROR("%s unable to allocate memory for config_t.\n", __func__);
        goto error;
    }

    config->sections = list_new(section_free);
    if (!config->sections) {
        OSI_TRACE_ERROR("%s unable to allocate list for sections.\n", __func__);
        goto error;
    }

    return config;

error:;
    config_free(config);
    return NULL;
}

config_t *config_new(const char *filename)
{
   assert(filename != NULL);

   config_t *config = config_new_empty();
   if (!config) {
        return NULL;
   }

    config_parse(config);

    return config;

}

void config_free(config_t *config)
{
    if (!config) {
        return;
    }

    list_free(config->sections);
    osi_free(config);
}

bool config_has_section(const config_t *config, const char *section)
{
    assert(config != NULL);
    assert(section != NULL);

    return (section_find(config, section) != NULL);
}

bool config_has_key(const config_t *config, const char *section, const char *key)
{
    assert(config != NULL);
    assert(section != NULL);
    assert(key != NULL);

    return (entry_find(config, section, key) != NULL);
}

bool config_has_key_in_section(config_t *config, const char *key, char *key_value)
{
    OSI_TRACE_DEBUG("key = %s, value = %s", key, key_value);
    for (const list_node_t *node = list_begin(config->sections); node != list_end(config->sections); node = list_next(node)) {
        const section_t *section = (const section_t *)list_node(node);

        for (const list_node_t *node = list_begin(section->entries); node != list_end(section->entries); node = list_next(node)) {
            entry_t *entry = list_node(node);
            if (entry && entry->key && entry->value) {
                OSI_TRACE_DEBUG("entry->key = %s, entry->value = %s", entry->key, entry->value);
                if (!strcmp(entry->key, key) && !strcmp(entry->value, key_value)) {
                    OSI_TRACE_DEBUG("%s, the irk aready in the flash.", __func__);
                    return true;
                }
            }
        }
    }

    return false;
}

int config_get_int(const config_t *config, const char *section, const char *key, int def_value)
{
    assert(config != NULL);
    assert(section != NULL);
    assert(key != NULL);

    entry_t *entry = entry_find(config, section, key);
    if (!entry || !entry->value) {
        return def_value;
    }

    char *endptr;
    int ret = strtol(entry->value, &endptr, 0);
    return (*endptr == '\0') ? ret : def_value;
}

bool config_get_bool(const config_t *config, const char *section, const char *key, bool def_value)
{
    assert(config != NULL);
    assert(section != NULL);
    assert(key != NULL);

    entry_t *entry = entry_find(config, section, key);
    if (!entry || !entry->value) {
        return def_value;
    }

    if (!strcmp(entry->value, "true")) {
        return true;
    }
    if (!strcmp(entry->value, "false")) {
        return false;
    }

    return def_value;
}

const char *config_get_string(const config_t *config, const char *section, const char *key, const char *def_value)
{
    assert(config != NULL);
    assert(section != NULL);
    assert(key != NULL);

    entry_t *entry = entry_find(config, section, key);
    if (!entry) {
        return def_value;
    }

    return entry->value;
}

bool config_set_int(config_t *config, const char *section, const char *key, int value)
{
    assert(config != NULL);
    assert(section != NULL);
    assert(key != NULL);

    char value_str[32] = { 0 };
    sprintf(value_str, "%d", value);
    return config_set_string(config, section, key, value_str, false);
}

bool config_set_bool(config_t *config, const char *section, const char *key, bool value)
{
    assert(config != NULL);
    assert(section != NULL);
    assert(key != NULL);

    return config_set_string(config, section, key, value ? "true" : "false", false);
}

bool config_set_string(config_t *config, const char *section, const char *key, const char *value, bool insert_back)
{
    bool ret = false;
    section_t *sec = section_find(config, section);
    if (!sec) {
        sec = section_new(section);
        if (!sec) {
            return false;
        } else {
            if (insert_back) {
                ret = list_append(config->sections, sec);
            } else {
                ret = list_prepend(config->sections, sec);
            }

            if (!ret)
                return ret;
        }
    }

    for (const list_node_t *node = list_begin(sec->entries); node != list_end(sec->entries); node = list_next(node)) {
        entry_t *entry = list_node(node);
        if (!strcmp(entry->key, key)) {
            osi_free(entry->value);
            entry->value = osi_strdup(value);
            return entry->value? true : false;
        }
    }

    entry_t *entry = entry_new(key, value);
    if (entry == NULL) {
        ret = false;
    } else {
        ret = list_append(sec->entries, entry);
    }

    return ret;
}

bool config_remove_section(config_t *config, const char *section)
{
    assert(config != NULL);
    assert(section != NULL);

    section_t *sec = section_find(config, section);
    if (!sec) {
        return false;
    }

    return list_remove(config->sections, sec);
}

bool config_remove_key(config_t *config, const char *section, const char *key)
{
    assert(config != NULL);
    assert(section != NULL);
    assert(key != NULL);

    section_t *sec = section_find(config, section);
    entry_t *entry = entry_find(config, section, key);
    if (!sec || !entry) {
        return false;
    }

    return list_remove(sec->entries, entry);
}

const config_section_node_t *config_section_begin(const config_t *config)
{
    assert(config != NULL);
    return (const config_section_node_t *)list_begin(config->sections);
}

const config_section_node_t *config_section_end(const config_t *config)
{
    assert(config != NULL);
    return (const config_section_node_t *)list_end(config->sections);
}

const config_section_node_t *config_section_next(const config_section_node_t *node)
{
    assert(node != NULL);
    return (const config_section_node_t *)list_next((const list_node_t *)node);
}

const char *config_section_name(const config_section_node_t *node)
{
    assert(node != NULL);
    const list_node_t *lnode = (const list_node_t *)node;
    const section_t *section = (const section_t *)list_node(lnode);
    return section->name;
}

static int get_config_size(const config_t *config)
{
    assert(config != NULL);

    int w_len = 0, total_size = 0, num = 0;

    for (const list_node_t *node = list_begin(config->sections); (node != list_end(config->sections)) && (num < CONFIG_BLUEDROID_MAX_STORE_VALUE); node = list_next(node), num ++) {
        const section_t *section = (const section_t *)list_node(node);
        w_len = strlen(section->name) + strlen("[]\n");// format "[section->name]\n"
        total_size += w_len;
        for (const list_node_t *enode = list_begin(section->entries); enode != list_end(section->entries); enode = list_next(enode)) {
            const entry_t *entry = (const entry_t *)list_node(enode);
            if (entry && entry->key && entry->value) {
                w_len = strlen(entry->key) + strlen(entry->value) + strlen(" = \n");// format "entry->key = entry->value\n"
                total_size += w_len;
            }
        }

        // Only add a separating newline if there are more sections.
        if (list_next(node) != list_end(config->sections)) {
                total_size ++;  //'\n'
        } else {
            break;
        }
    }
    total_size ++; //'\0'
    return total_size;
}

bool config_save(const config_t *config, const char *filename)
{
#if CONFIG_BLUEDROID_USE_SETTINGS
    assert(config != NULL);
#ifdef FILENAME_NEED
    assert(filename != NULL);
    assert(*filename != '\0');
#else
    (void)filename;
#endif

    int err_code = 0;
    int num = 0;
    char *line = osi_calloc(1024);
    const size_t keyname_bufsz = sizeof(CONFIG_KEY) + 5 + 1; // including log10(sizeof(i))
    char *keyname = osi_calloc(keyname_bufsz);
    int config_size = 0;
    char *buf = NULL;

    config_size = get_config_size(config);
    buf = osi_calloc(config_size + 100);
    if (!line || !buf || !keyname) {
        err_code |= 0x01;
        goto error;
    }

    memset(line, 0, 1024);
    memset(keyname, 0, keyname_bufsz);
    memset(buf, 0, config_size + 100);
    int w_cnt, w_cnt_total = 0, name_length = 0;

    for (const list_node_t *node = list_begin(config->sections); (node != list_end(config->sections)) && (num < CONFIG_BLUEDROID_MAX_STORE_VALUE); node = list_next(node), num ++) {
        const section_t *section = (const section_t *)list_node(node);
        w_cnt = snprintf(line, 1024, "[%s]\n", section->name);
        OSI_TRACE_DEBUG("section name: %s, w_cnt + w_cnt_total = %d\n", section->name, w_cnt + w_cnt_total);
        memcpy(buf + w_cnt_total, line, w_cnt);
        w_cnt_total += w_cnt;
        for (const list_node_t *enode = list_begin(section->entries); enode != list_end(section->entries); enode = list_next(enode)) {
            const entry_t *entry = (const entry_t *)list_node(enode);
            if (entry && entry->key && entry->value) {
                OSI_TRACE_DEBUG("(key, val): (%s, %s)\n", entry->key, entry->value);
                w_cnt = snprintf(line, 1024, "%s = %s\n", entry->key, entry->value);
                OSI_TRACE_DEBUG("%s, w_cnt + w_cnt_total = %d", __func__, w_cnt + w_cnt_total);
                memcpy(buf + w_cnt_total, line, w_cnt);
                w_cnt_total += w_cnt;
            }
        }

        // Only add a separating newline if there are more sections.
        if (list_next(node) != list_end(config->sections)) {
            buf[w_cnt_total] = '\n';
            w_cnt_total += 1;
        } else {
            break;
        }
    }
    buf[w_cnt_total] = '\0';
    if (w_cnt_total < CONFIG_FILE_MAX_SIZE) {
        name_length = snprintf(keyname, keyname_bufsz, "%s%d", CONFIG_KEY, 0);
    }else {
        uint count = (w_cnt_total / CONFIG_FILE_MAX_SIZE);
        for (int i = 0; i <= count; i++)
        {
            name_length = snprintf(keyname, keyname_bufsz, "%s%d", CONFIG_KEY, i);
        }
    }

    settings_save_one(keyname, buf, name_length + w_cnt_total);

    osi_free(line);
    osi_free(buf);
    osi_free(keyname);
    return true;

error:
    if (buf) {
        osi_free(buf);
    }
    if (line) {
        osi_free(line);
    }
    if (keyname) {
        osi_free(keyname);
    }
    if (err_code) {
        OSI_TRACE_ERROR("%s, err_code: 0x%x\n", __func__, err_code);
    }
    return false;
#else
    return true;
#endif
}

static char *trim(char *str)
{
    while (isspace((unsigned char)(*str))) {
        ++str;
    }

    if (!*str) {
        return str;
    }

    char *end_str = str + strlen(str) - 1;
    while (end_str > str && isspace((unsigned char)(*end_str))) {
        --end_str;
    }

    end_str[1] = '\0';
    return str;
}

#if CONFIG_BLUEDROID_USE_SETTINGS

static int cfg_rd_cb(    const char      *key,
    size_t           len,
    settings_read_cb read_cb,
    void            *cb_arg,
    void            *param)
{
    cfg_t *p = NULL;
    p = (cfg_t *)param;
    assert(p->buf == NULL);

    p->buf = (char *)osi_calloc(len);
    if (p->buf == NULL) {
        printf("CONFIG ERR:Alloc buffer error\n");
        return -1;
    }
    memset(p->buf, 0, len);
    if (len) {
        p->len = read_cb(cb_arg, (void *)(p->buf), len);
    }
    return 0;
}
#endif

static void config_parse(config_t *config)
{
#if CONFIG_BLUEDROID_USE_SETTINGS
    assert(config != NULL);

    int err;
    int line_num = 0;
    int err_code = 0;
    uint16_t i = 0;
    size_t length = CONFIG_FILE_DEFAULE_LENGTH;
    size_t total_length = 0;
    char *line = osi_calloc(1024);
    char *section = osi_calloc(1024);
    const size_t keyname_bufsz = sizeof(CONFIG_KEY) + 5 + 1; // including log10(sizeof(i))
    char *keyname = osi_calloc(keyname_bufsz);
    cfg_t cfg = {
        .len = 0,
        .buf = NULL,
    };

    if (!line || !section || !keyname) {
        err_code |= 0x01;
        goto error;
    }
    snprintf(keyname, keyname_bufsz, "%s%d", CONFIG_KEY, 0);
    settings_load_subtree_direct(keyname, cfg_rd_cb, (void *)&cfg);

    if (cfg.buf == NULL) {
        printf("Warning: bt config parse failed\n");
        goto error;
    }

    char *p_line_end;
    char *p_line_bgn = cfg.buf;
    strcpy(section, CONFIG_DEFAULT_SECTION);

    total_length += cfg.len;
    while ((p_line_bgn < cfg.buf + total_length - 1) && (p_line_end = strchr(p_line_bgn, '\n'))) {

        // get one line
        int line_len = p_line_end - p_line_bgn;
        if (line_len > 1023) {
            OSI_TRACE_WARNING("%s exceed max line length on line %d.\n", __func__, line_num);
            break;
        }
        memcpy(line, p_line_bgn, line_len);
        line[line_len] = '\0';
        p_line_bgn = p_line_end + 1;
        char *line_ptr = trim(line);
        ++line_num;

        // Skip blank and comment lines.
        if (*line_ptr == '\0' || *line_ptr == '#') {
            continue;
        }

        if (*line_ptr == '[') {
            size_t len = strlen(line_ptr);
            if (line_ptr[len - 1] != ']') {
                OSI_TRACE_WARNING("%s unterminated section name on line %d.\n", __func__, line_num);
                continue;
            }
            strncpy(section, line_ptr + 1, len - 2);
            section[len - 2] = '\0';
        } else {
            char *split = strchr(line_ptr, '=');
            if (!split) {
                OSI_TRACE_DEBUG("%s no key/value separator found on line %d.\n", __func__, line_num);
                continue;
            }
            *split = '\0';
            config_set_string(config, section, trim(line_ptr), trim(split + 1), true);
        }
    }

error:
    if (cfg.buf) {
        osi_free(cfg.buf);
    }
    if (line) {
        osi_free(line);
    }
    if (section) {
        osi_free(section);
    }
    if (keyname) {
        osi_free(keyname);
    }
    if (err_code) {
        OSI_TRACE_ERROR("%s returned with err code: %d\n", __func__, err_code);
    }
#endif
}

static section_t *section_new(const char *name)
{
    section_t *section = osi_calloc(sizeof(section_t));
    if (!section) {
        return NULL;
    }

    section->name = osi_strdup(name);
    if (section->name == NULL) {
        osi_free(section);
        return NULL;
    }

    section->entries = list_new(entry_free);
    if (section->entries == NULL) {
        osi_free(section);
        osi_free(section->name);
        return NULL;
    }

    return section;
}

static void section_free(void *ptr)
{
    if (!ptr) {
        return;
    }

    section_t *section = ptr;
    osi_free(section->name);
    list_free(section->entries);
    osi_free(section);
}

static section_t *section_find(const config_t *config, const char *section)
{
    for (const list_node_t *node = list_begin(config->sections); node != list_end(config->sections); node = list_next(node)) {
        section_t *sec = list_node(node);
        if (!strcmp(sec->name, section)) {
            return sec;
        }
    }

    return NULL;
}

static entry_t *entry_new(const char *key, const char *value)
{
    entry_t *entry = osi_calloc(sizeof(entry_t));
    if (!entry) {
        return NULL;
    }

    entry->key = osi_strdup(key);
    if (entry->key == NULL) {
        osi_free(entry);
        return NULL;
    }

    entry->value = osi_strdup(value);
    if (entry->value == NULL) {
        osi_free(entry);
        osi_free(entry->key);
        return NULL;
    }

    return entry;
}

static void entry_free(void *ptr)
{
    if (!ptr) {
        return;
    }

    entry_t *entry = ptr;
    osi_free(entry->key);
    osi_free(entry->value);
    osi_free(entry);
}

static entry_t *entry_find(const config_t *config, const char *section, const char *key)
{
    section_t *sec = section_find(config, section);
    if (!sec) {
        return NULL;
    }

    for (const list_node_t *node = list_begin(sec->entries); node != list_end(sec->entries); node = list_next(node)) {
        entry_t *entry = list_node(node);
        if (!strcmp(entry->key, key)) {
            return entry;
        }
    }

    return NULL;
}
