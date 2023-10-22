/*
 * Copyright (c) 2020 Xradio Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdbool.h>
#include <zephyr.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>

#include "settings/settings.h"
#include "include/settings/settings_file.h"
#include "settings_priv.h"

#define LOG_MODULE_NAME settings_files
#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_SETTINGS)
#include "common/log.h"

#define MAX_PATH_LEN	128

int32_t settings_backend_init(void);
void settings_mount_fs_backend(struct settings_file *cf);

static int32_t settings_file_load(struct settings_store *cs,
				  const struct settings_load_arg *arg);
static int32_t settings_file_save(struct settings_store *cs, const char *name,
				  const char *value, size_t val_len);

static const struct settings_store_itf settings_file_itf = {
	.csi_load = settings_file_load,
	.csi_save = settings_file_save,
};

/*
 * Register a file to be a source of configuration.
 */
int32_t settings_file_src(struct settings_file *cf)
{
	if (!cf->cf_name) {
		return -EINVAL;
	}
	cf->cf_store.cs_itf = &settings_file_itf;
	settings_src_register(&cf->cf_store);

	return 0;
}

/*
 * Register a file to be a destination of configuration.
 */
int32_t settings_file_dst(struct settings_file *cf)
{
	if (!cf->cf_name) {
		return -EINVAL;
	}
	cf->cf_store.cs_itf = &settings_file_itf;
	settings_dst_register(&cf->cf_store);

	return 0;
}

#if defined(CONFIG_BT_DEINIT)
int32_t settings_file_src_deinit(struct settings_file *cf)
{
	if (!cf->cf_name) {
		return -EINVAL;
	}
	cf->cf_store.cs_itf = &settings_file_itf;
	settings_src_unregister(&cf->cf_store);

	return 0;
}

int32_t settings_file_dst_deinit(struct settings_file *cf)
{
	if (!cf->cf_name) {
		return -EINVAL;
	}
	cf->cf_store.cs_itf = &settings_file_itf;
	settings_dst_unregister(&cf->cf_store);

	return 0;
}
#endif

/**
 * @brief Check if there is any duplicate of the current setting
 *
 * This function checks if there is any duplicated data further in the buffer.
 *
 * @param entry_ctx Current entry context
 * @param name		The name of the current entry
 *
 * @retval false No duplicates found
 * @retval true  Duplicate found
 */
static bool settings_file_check_duplicate(
				  const struct line_entry_ctx *entry_ctx,
				  const char * const name)
{
	struct line_entry_ctx entry2_ctx = *entry_ctx;

	/* Searching the duplicates */
	while (settings_next_line_ctx(&entry2_ctx) == 0) {
		char name2[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
		size_t name2_len;

		if (entry2_ctx.len == 0) {
			break;
		}

		if (settings_line_name_read(name2, sizeof(name2), &name2_len,
						&entry2_ctx)) {
			continue;
		}
		name2[name2_len] = '\0';

		if (!strcmp(name, name2)) {
			return true;
		}
	}
	return false;
}

static int32_t read_entry_len(const struct line_entry_ctx *entry_ctx, off_t off)
{
	if (off >= entry_ctx->len) {
		return 0;
	}
	return entry_ctx->len - off;
}

static int32_t settings_file_load_priv(struct settings_store *cs, line_load_cb cb,
				   void *cb_arg, bool filter_duplicates)
{
	struct settings_file *cf = (struct settings_file *)cs;
	int fd;
	int lines;
	int rc;
	struct line_entry_ctx entry_ctx;

	lines = 0;

	fd = open(cf->cf_name, O_RDWR | O_CREAT);
	if (fd < 0) {
		return -EINVAL;
	}
	entry_ctx.stor_ctx = fd;
	entry_ctx.seek = 0;
	entry_ctx.len = 0;

	while (1) {
		char name[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
		size_t name_len;
		bool pass_entry = true;

		rc = settings_next_line_ctx(&entry_ctx);
		if (rc || entry_ctx.len == 0) {
			break;
		}

		rc = settings_line_name_read(name, sizeof(name), &name_len,
						 &entry_ctx);
		if (rc || name_len == 0) {
			break;
		}
		name[name_len] = '\0';

		if (filter_duplicates &&
			(!read_entry_len(&entry_ctx, name_len+1) ||
			 settings_file_check_duplicate(&entry_ctx, name))) {
			pass_entry = false;
		}
		/*name, val-read_cb-ctx, val-off*/
		/* take into account '=' separator after the name */
		if (pass_entry) {
			cb(name, (void *)&entry_ctx, name_len + 1, cb_arg);
		}
		lines++;
	}

	rc = close(fd);

	cf->cf_lines = lines;

	return rc;
}


/*
 * Called to load configuration items.
 */
static int32_t settings_file_load(struct settings_store *cs,
				  const struct settings_load_arg *arg)
{
	return settings_file_load_priv(cs,
					   settings_line_load_cb,
					   (void *)arg,
					   true);
}

static void settings_tmpfile(char *dst, const char *src, char *pfx)
{
	int len;
	int pfx_len;

	len = strlen(src);
	pfx_len = strlen(pfx);
	if (len + pfx_len >= SETTINGS_FILE_NAME_MAX) {
		len = SETTINGS_FILE_NAME_MAX - pfx_len - 1;
	}
	memcpy(dst, src, len);
	memcpy(dst + len, pfx, pfx_len);
	dst[len + pfx_len] = '\0';
}

static int32_t settings_file_create_or_replace(int *p_fd,
					   const char *file_name)//todo
{
	struct stat entry;

	if (stat(file_name, &entry) == 0) {
		if (entry.st_mode == S_IFDIR) {
			if (unlink(file_name)) {
				return -EIO;
			}
		} else {
			return -EISDIR;
		}
	}
	*p_fd = open(file_name, O_RDWR | O_CREAT);
	if (*p_fd < 0) {
		return -1;
	}
	return 0;
}

/*
 * Try to compress configuration file by keeping unique names only.
 */
static int32_t settings_file_save_and_compress(struct settings_file *cf,
			   const char *name, const char *value,
			   size_t val_len)
{
	int rc, rc2, rc3;
	int read_fd;
	int write_fd;
	char tmp_file[SETTINGS_FILE_NAME_MAX];
	char name1[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	char name2[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	struct line_entry_ctx loc1;
	struct line_entry_ctx loc2;
	struct line_entry_ctx loc3;

	int copy;
	int lines;
	size_t new_name_len;
	size_t val1_off;

	read_fd = open(cf->cf_name, O_RDWR | O_CREAT);
	if (read_fd < 0) {
		return -ENOEXEC;
	}

	settings_tmpfile(tmp_file, cf->cf_name, ".cmp");

	if (settings_file_create_or_replace(&write_fd, tmp_file)) {//todo
		close(read_fd);
		return -ENOEXEC;
	}

	loc1.stor_ctx = read_fd;
	loc1.seek = 0;
	loc1.len = 0;
	loc3.stor_ctx = write_fd;
	lines = 0;
	new_name_len = strlen(name);

	while (1) {
		rc = settings_next_line_ctx(&loc1);

		if (rc || loc1.len == 0) {
			/* try to amend new value to the commpresed file */
			break;
		}

		rc = settings_line_name_read(name1, sizeof(name1), &val1_off,
						 &loc1);
		if (rc) {
			/* try to process next line */
			continue;
		}

		if (val1_off + 1 == loc1.len) {
			/* Lack of a value so the record is a deletion-record */
			/* No sense to copy empty entry from */
			/* the oldest sector */
			continue;
		}

		/* avoid copping value which will be overwritten by new value*/
		if ((val1_off == new_name_len) &&
			!memcmp(name1, name, val1_off)) {
			continue;
		}

		loc2 = loc1;

		copy = 1;
		while (1) {
			size_t val2_off;

			rc = settings_next_line_ctx(&loc2);

			if (rc || loc2.len == 0) {
				/* try to amend new value to */
				/* the commpresed file */
				break;
			}

			rc = settings_line_name_read(name2, sizeof(name2),
							 &val2_off, &loc2);
			if (rc) {
				/* try to process next line */
				continue;
			}
			if ((val1_off == val2_off) &&
				!memcmp(name1, name2, val1_off)) {
				copy = 0; /* newer version doesn't exist */
				break;
			}
		}
		if (!copy) {
			continue;
		}

		loc2 = loc1;
		loc2.len += 2;
		loc2.seek -= 2;
		rc = settings_line_entry_copy(&loc3, 0, &loc2, 0, loc2.len);
		if (rc) {
			/* compressed file might be corrupted */
			goto end_rolback;
		}

		lines++;
	}

	/* at last store the new value */
	rc = settings_line_write(name, value, val_len, 0, &loc3);
	if (rc) {
		/* compressed file might be corrupted */
		goto end_rolback;
	}

	rc = close(write_fd);
	rc2 = close(read_fd);
	rc3 = unlink(cf->cf_name);//todo

	if (rc == 0 && rc2 == 0 && rc3 == 0) {
		rc3 = rename(tmp_file, cf->cf_name);//todo
		if (rc3) {
			return -ENOENT;
		}
		cf->cf_lines = lines + 1;
	} else {
		rc = -EIO;
	}
	/*
	 * XXX at settings_file_load(), look for .cmp if actual file does not
	 * exist.
	 */
	return 0;
end_rolback:
	close(write_fd);
	rc3 = close(read_fd);
	if (rc3 == 0) {
		unlink(tmp_file);//todo
	}

	return -EIO;
}

static int32_t settings_file_save_priv(struct settings_store *cs, const char *name,
				   const char *value, size_t val_len)
{
	struct settings_file *cf = (struct settings_file *)cs;
	struct line_entry_ctx entry_ctx;
	int fd;
	int rc2;
	int rc;

	if (!name) {
		return -EINVAL;
	}

	if (cf->cf_maxlines && (cf->cf_lines + 1 >= cf->cf_maxlines)) {
		/*
		 * Compress before config file size exceeds
		 * the max number of lines.
		 */
		return settings_file_save_and_compress(cf, name, value,
							   val_len);
	}

	/*
	 * Open the file to add this one value.
	 */
	fd = open(cf->cf_name, O_RDWR | O_CREAT);
	if (fd < 0) {
		return -1;
	}

	rc = lseek(fd, 0, SEEK_END);//todo
	if (rc >= 0) {
		entry_ctx.stor_ctx = fd;
		rc = settings_line_write(name, value, val_len, 0,
					  (void *)&entry_ctx);
		if (rc == 0) {
			cf->cf_lines++;
		}
	}
	rc2 = close(fd);
	if (rc == 0) {
		rc = rc2;
	}

	return rc;
}


/*
 * Called to save configuration.
 */
static int32_t settings_file_save(struct settings_store *cs, const char *name,
				  const char *value, size_t val_len)
{
	struct settings_line_dup_check_arg cdca;

	if (val_len > 0 && value == NULL) {
		return -EINVAL;
	}

	/*
	 * Check if we're writing the same value again.
	 */
	cdca.name = name;
	cdca.val = (char *)value;
	cdca.is_dup = 0;
	cdca.val_len = val_len;
	settings_file_load_priv(cs, settings_line_dup_check_cb, &cdca, false);
	if (cdca.is_dup == 1) {
		return 0;
	}
	return settings_file_save_priv(cs, name, (char *)value, val_len);
}

static int32_t read_handler(void *ctx, off_t off, char *buf, size_t *len)
{
	struct line_entry_ctx *entry_ctx = ctx;
	int fd = entry_ctx->stor_ctx;
	ssize_t r_len;
	int rc;

	/* 0 is reserved for reding the length-field only */
	if (entry_ctx->len != 0) {
		if (off >= entry_ctx->len) {
			*len = 0;
			return 0;
		}

		if ((off + *len) > entry_ctx->len) {
			*len = entry_ctx->len - off;
		}
	}

	rc = lseek(fd, entry_ctx->seek + off, SEEK_SET);//todo
	if (rc < 0) {
		goto end;
	}

	r_len = read(fd, buf, *len);

	if (r_len >= 0) {
		*len = r_len;
		rc = 0;
	} else {
		rc = r_len;
	}
end:
	return rc;
}

static size_t get_len_cb(void *ctx)
{
	struct line_entry_ctx *entry_ctx = ctx;

	return entry_ctx->len;
}

static int32_t write_handler(void *ctx, off_t off, char const *buf, size_t len)
{
	struct line_entry_ctx *entry_ctx = ctx;
	int fd = entry_ctx->stor_ctx;
	int rc;

	/* append to file only */
	rc = lseek(fd, 0, SEEK_END);//todo
	if (rc >= 0) {
		rc = write(fd, buf, len);
		if (rc > 0) {
			rc = 0;
		}
	}

	return rc;
}

void settings_mount_fs_backend(struct settings_file *cf)
{
	settings_line_io_init(read_handler, write_handler, get_len_cb, 1);
}

#if IS_ENABLED(CONFIG_SETTINGS)
#if defined(CONFIG_BT_DEINIT)
static struct settings_file config_init_settings_file = {
	.cf_name = CONFIG_SETTINGS_FS_FILE,
	.cf_maxlines = CONFIG_SETTINGS_FS_MAX_LINES,
};
#endif

int32_t settings_backend_init(void)
{
#if !defined(CONFIG_BT_DEINIT)
	static struct settings_file config_init_settings_file = {
		.cf_name = CONFIG_SETTINGS_FS_FILE,
		.cf_maxlines = CONFIG_SETTINGS_FS_MAX_LINES
	};
#endif
	int rc;

	rc = settings_file_src(&config_init_settings_file);
	if (rc) {
		__ASSERT(0, "Setting file source failed! err(%d)\n", rc);
	}

	rc = settings_file_dst(&config_init_settings_file);
	if (rc) {
		__ASSERT(0, "Setting file destination failed! err(%d)\n", rc);
	}

	settings_mount_fs_backend(&config_init_settings_file);

	rc = mkdir(CONFIG_SETTINGS_FS_DIR,0777);

	/*
	 * The following lines mask the file exist error.
	 */
	if (rc == -EEXIST) {
		rc = 0;
	}

	return rc;
}

#if defined(CONFIG_BT_DEINIT)
int settings_unmount_fs_backend(struct settings_file *cf)
{
	settings_line_io_deinit();
	return 0;
}

int settings_backend_deinit(void)
{
	int rc;

	settings_unmount_fs_backend(&config_init_settings_file);

	rc = settings_file_dst_deinit(&config_init_settings_file);
	if (rc) {
		return rc;
	}

	rc = settings_file_src_deinit(&config_init_settings_file);
	if (rc) {
		return rc;
	}

	return 0;
}
#endif
#endif
