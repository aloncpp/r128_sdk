/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <errno.h>

#include "settings/settings.h"
#include "settings_priv.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_SETTINGS)
#define LOG_MODULE_NAME settings_stores
#include "common/log.h"

sys_slist_t settings_load_srcs;
struct settings_store *settings_save_dst;
extern struct k_mutex settings_lock;

#if !defined(CONFIG_BLEHOST_Z_ITERABLE_SECTION)
extern sys_slist_t settings_handler_static_list;
#endif

void settings_src_register(struct settings_store *cs)
{
	sys_slist_append(&settings_load_srcs, &cs->cs_next);
}

void settings_dst_register(struct settings_store *cs)
{
	settings_save_dst = cs;
}

#if defined(CONFIG_BT_DEINIT)
int settings_src_unregister(struct settings_store *cs)
{
	struct settings_store *cs_find;

	k_mutex_lock(&settings_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&settings_load_srcs, cs_find, cs_next) {
		if (cs_find->cs_itf == cs->cs_itf) {
			sys_slist_find_and_remove(&settings_load_srcs, &cs_find->cs_next);
		}
	}
	k_mutex_unlock(&settings_lock);

	return 0;
}

int settings_dst_unregister(struct settings_store *cs)
{
	if (settings_save_dst == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&settings_lock, K_FOREVER);
	settings_save_dst = NULL;
	k_mutex_unlock(&settings_lock);

	return 0;
}
#endif

int settings_load(void)
{
	return settings_load_subtree(NULL);
}

int settings_load_subtree(const char *subtree)
{
	struct settings_store *cs;
	int rc;
	const struct settings_load_arg arg = {
		.subtree = subtree
	};

	/*
	 * for every config store
	 *    load config
	 *    apply config
	 *    commit all
	 */
	k_mutex_lock(&settings_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&settings_load_srcs, cs, cs_next) {
		cs->cs_itf->csi_load(cs, &arg);
	}
	rc = settings_commit_subtree(subtree);
	k_mutex_unlock(&settings_lock);
	return rc;
}

int settings_load_subtree_direct(
	const char             *subtree,
	settings_load_direct_cb cb,
	void                   *param)
{
	struct settings_store *cs;

	const struct settings_load_arg arg = {
		.subtree = subtree,
		.cb = cb,
		.param = param
	};
	/*
	 * for every config store
	 *    load config
	 *    apply config
	 *    commit all
	 */
	k_mutex_lock(&settings_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&settings_load_srcs, cs, cs_next) {
		cs->cs_itf->csi_load(cs, &arg);
	}
	k_mutex_unlock(&settings_lock);
	return 0;
}

/*
 * Append a single value to persisted config. Don't store duplicate value.
 */
#if (CONFIG_BT_SETTINGS)
int settings_save_one(const char *name, const void *value, size_t val_len)
{
	int rc;
	struct settings_store *cs;

	cs = settings_save_dst;
	if (!cs) {
		return -ENOENT;
	}

	k_mutex_lock(&settings_lock, K_FOREVER);

	rc = cs->cs_itf->csi_save(cs, name, (char *)value, val_len);

	k_mutex_unlock(&settings_lock);

	return rc;
}
#endif

int settings_delete(const char *name)
{
	return settings_save_one(name, NULL, 0);
}

int settings_save(void)
{
	struct settings_store *cs;
	int rc;
#if defined(CONFIG_BLEHOST_Z_ITERABLE_SECTION) || defined(CONFIG_SETTINGS_DYNAMIC_HANDLERS)
	int rc2;
#endif

	cs = settings_save_dst;
	if (!cs) {
		return -ENOENT;
	}

	if (cs->cs_itf->csi_save_start) {
		cs->cs_itf->csi_save_start(cs);
	}
	rc = 0;

#if defined(CONFIG_BLEHOST_Z_ITERABLE_SECTION)
	Z_STRUCT_SECTION_FOREACH(settings_handler_static, ch) {
		if (ch->h_export) {
			rc2 = ch->h_export(settings_save_one);
			if (!rc) {
				rc = rc2;
			}
		}
	}
#endif

#if defined(CONFIG_SETTINGS_DYNAMIC_HANDLERS)
	struct settings_handler *ch;
	SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
		if (ch->h_export) {
			rc2 = ch->h_export(settings_save_one);
			if (!rc) {
				rc = rc2;
			}
		}
	}
#endif /* CONFIG_SETTINGS_DYNAMIC_HANDLERS */

	if (cs->cs_itf->csi_save_end) {
		cs->cs_itf->csi_save_end(cs);
	}
	return rc;
}

void settings_store_init(void)
{
	sys_slist_init(&settings_load_srcs);
}

#if defined(CONFIG_BT_DEINIT)
void settings_store_deinit(void)
{
	sys_slist_deinit(&settings_load_srcs);
}
#endif

