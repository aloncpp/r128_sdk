/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SETTINGS_FILE_H_
#define __SETTINGS_FILE_H_

#include "settings/settings.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SETTINGS_FILE_NAME_MAX 32 /* max length for settings filename */

struct settings_file {
	struct settings_store cf_store;
	const char *cf_name;	/* filename */
	int cf_maxlines;	/* max # of lines before compressing */
	int cf_lines;		/* private */
};

/* register file to be source of settings */
int32_t settings_file_src(struct settings_file *cf);

/* settings saves go to a file */
int32_t settings_file_dst(struct settings_file *cf);

#if defined(CONFIG_BT_DEINIT)
int32_t settings_file_src_deinit(struct settings_file *cf);
int32_t settings_file_dst_deinit(struct settings_file *cf);
#endif

void settings_mount_fs_backend(struct settings_file *cf);

#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_FILE_H_ */
