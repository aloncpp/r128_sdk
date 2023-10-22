#include "ramfs.h"

#ifndef MIN
#define MIN(a, b) (a > b ? b : a)
#endif

#define RAMFS_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))
#define RAMFS_ALIGN_SIZE (4)

static pthread_mutex_t _ramfs_lock;
static uint8_t *ramfs_pool = NULL;

#define ramfs_vfs_lock()          pthread_mutex_lock(&_ramfs_lock);
#define ramfs_vfs_unlock()        pthread_mutex_unlock(&_ramfs_lock);
#define ramfs_vfs_init()          pthread_mutex_init(&_ramfs_lock, NULL);

#define RAMFS_ASSERT(x) configASSERT(x)

static void *ramfs_get_entry(struct ramfs *ctx, int index)
{
    return ctx->files[index].entry;
}

static char *ramfs_get_path(struct ramfs *ctx, int index)
{
    return ctx->files[index].path;
}

static void ramfs_add_file(struct ramfs *ctx, int index, void *entry,
                           const char *path)
{
    ctx->files[index].entry = entry;
    ctx->files[index].path = strdup(path);
}

static void ramfs_del_file(struct ramfs *ctx, int index)
{
    ctx->files[index].entry = NULL;
    free(ctx->files[index].path);
    ctx->files[index].path = NULL;
}

static int ramfs_statfs(void *_ctx, const char *path, struct statfs *buf)
{
    struct ramfs *ramfs;

    ramfs = (struct ramfs *)_ctx;
    RAMFS_ASSERT(ramfs != NULL);
    RAMFS_ASSERT(buf != NULL);

    ramfs_vfs_lock();

    buf->f_bsize  = 512;
    buf->f_blocks = ramfs->memheap.pool_size / 512;
    buf->f_bfree  = ramfs->memheap.available_size / 512;
    buf->f_bavail = ramfs->memheap.available_size / 512;

    ramfs_vfs_unlock();

    return 0;
}

static int ramfs_fstatfs(void *ctx, int fd, struct statfs *sfs)
{
    return ramfs_statfs(ctx, ramfs_get_path(ctx, fd), sfs);
}

struct ramfs_dirent *ramfs_lookup_dentry(struct ramfs *ramfs,
        const char *path,
        size_t  *size,
        struct ramfs_dirent *parent_dirent)
{
    char name[RAMFS_NAME_MAX + 1];
    char *subpath = NULL;
    char *temp_path = (char *)path;
    struct ramfs_dirent *dirent;

    while (*temp_path == '/' && *temp_path)
    {
        temp_path ++;
    }

    memset(name, 0, sizeof(name));

    subpath = strchr(temp_path, '/');
    if (!subpath)
    {
        snprintf(name, RAMFS_NAME_MAX, "%s", temp_path);
    }
    else
    {
        snprintf(name, (subpath - temp_path + 1) > RAMFS_NAME_MAX ? RAMFS_NAME_MAX : (subpath - temp_path + 1), "%s", temp_path);
    }

    if (parent_dirent->dirent_list.next == &(parent_dirent->dirent_list))
    {
        return NULL;
    }

    for (dirent = list_entry(parent_dirent->dirent_list.next, struct ramfs_dirent, list);
         dirent->list.next != &(parent_dirent->dirent_list);
         dirent = list_entry(dirent->list.next, struct ramfs_dirent, list))
    {
        if (strcmp(dirent->name, name) == 0)
        {
            if (dirent->type == RAMFS_DIR && subpath)
            {
                return ramfs_lookup_dentry(ramfs, subpath, size, dirent);
            }
            return dirent;
        }
    }

    if (strcmp(dirent->name, name) == 0)
    {
        if (dirent->type == RAMFS_DIR && subpath)
        {
            return ramfs_lookup_dentry(ramfs, subpath, size, dirent);
        }
        return dirent;
    }

    return NULL;
}

struct ramfs_dirent *ramfs_lookup_partent_dentry(struct ramfs *ramfs,
        const char       *path)
{
    char name[RAMFS_NAME_MAX + 1];
    char *subpath = NULL;
    struct ramfs_dirent *dirent;
    size_t size;

    subpath = strrchr(path, '/');
    if (!subpath || subpath == path)
    {
        return &ramfs->root;
    }
    else
    {
        memset(name, 0, sizeof(name));
        memcpy(name, path, subpath - path);
        return ramfs_lookup_dentry(ramfs, name, &size, &ramfs->root);
    }
}

struct ramfs_dirent *ramfs_lookup(struct ramfs *ramfs,
                                  const char *path,
                                  size_t *size)
{
    const char *subpath;
    struct ramfs_dirent *dirent;

    subpath = path;
    while (*subpath == '/' && *subpath)
    {
        subpath ++;
    }
    if (! *subpath) /* is root directory */
    {
        *size = 0;

        return &(ramfs->root);
    }

    ramfs_vfs_lock();

    dirent = ramfs_lookup_dentry(ramfs, path, size, &ramfs->root);

    ramfs_vfs_unlock();
    return dirent;
}

static ssize_t ramfs_read(void *_ctx, int fd, void *buf, size_t count)
{
    size_t length;
    struct ramfs_dirent *dirent;

    struct ramfs *ramfs = _ctx;
    ramfs_file_t *file = ramfs_get_entry(_ctx, fd);

    ramfs_vfs_lock();

    dirent = (struct ramfs_dirent *)file->data;
    RAMFS_ASSERT(dirent != NULL);

    if (count < file->size - file->pos)
    {
        length = count;
    }
    else
    {
        length = file->size - file->pos;
    }

    if (length > 0)
    {
        int slice_pos = file->pos / DATA_SLICE_SIZE;
        int slice_inside_pos = file->pos % DATA_SLICE_SIZE;
        int size = length;
        int sz = 0;

        data_slice *data_s = slist_entry(dirent->data_slice_chain.list.next, data_slice, list);

        while (slice_pos-- > 0 && data_s != NULL)
        {
            if (data_s->list.next)
            {
                data_s = slist_entry(data_s->list.next, data_slice, list);
            }
            else
            {
                data_s = NULL;
            }
        }

        if (file->pos % DATA_SLICE_SIZE && data_s != NULL)
        {
            int len = MIN(DATA_SLICE_SIZE - slice_inside_pos, length);
            memcpy(buf, data_s->data_node + slice_inside_pos, len);
            size -= len;
            buf += len;
            sz += len;
            if (data_s->list.next)
            {
                data_s = slist_entry(data_s->list.next, data_slice, list);
            }
            else
            {
                data_s = NULL;
            }
        }

        while (size >= DATA_SLICE_SIZE && data_s != NULL)
        {
            memcpy(buf, data_s->data_node, DATA_SLICE_SIZE);
            size -= DATA_SLICE_SIZE;
            buf += DATA_SLICE_SIZE;
            sz += DATA_SLICE_SIZE;
            if (data_s->list.next)
            {
                data_s = slist_entry(data_s->list.next, data_slice, list);
            }
            else
            {
                data_s = NULL;
            }
        }

        if (size && data_s != NULL)
        {
            memcpy(buf, data_s->data_node, size);
            sz += size;
        }

        length = sz;
    }

    /* update file current position */
    file->pos += length;

    ramfs_vfs_unlock();
    return length;
}

static int ramfs_data_slice_free(struct ramfs_dirent *dirent, data_slice *data)
{
    struct ramfs *ramfs;
    data_slice *temp;
    slist_t *pos;
    slist_t *next_pos;

    RAMFS_ASSERT(dirent != NULL);

    ramfs = dirent->fs;
    RAMFS_ASSERT(ramfs != NULL);

    if (data == NULL)
    {
        return 0;
    }

    temp = data;
    pos = &(temp->list);
    next_pos = pos->next;
    while (next_pos)
    {
        pos = next_pos;
        temp = slist_entry(pos, data_slice, list);
        if (temp)
        {
            if (temp->data_node)
            {
                memheap_free(temp->data_node);
            }
            next_pos = pos->next;
            slist_remove(&(data->list), pos);
            free(temp);
        }
    }

    if (data->data_node)
    {
        memheap_free(data->data_node);
    }
    free(data);

    return 0;
}

static int ramfs_extend(ramfs_file_t *fd, size_t count)
{
    struct ramfs_dirent *dirent;
    struct ramfs *ramfs;

    dirent = (struct ramfs_dirent *)fd->data;
    RAMFS_ASSERT(dirent != NULL);

    ramfs = dirent->fs;
    RAMFS_ASSERT(ramfs != NULL);

    int inc_slice_num = 0;
    int i = 0;
    data_slice *data_s = NULL;
    data_slice *inc_add_root = NULL;
    slist_t *tail = NULL;

    if (count > 0)
    {
        int increase_size = count;
        int slice_inside_size = 0;

        if (fd->size % DATA_SLICE_SIZE)
        {
            slice_inside_size = DATA_SLICE_SIZE - (fd->size % DATA_SLICE_SIZE);
        }

        if (increase_size > slice_inside_size)
        {
            if (fd->size != 0)
            {
                increase_size -= slice_inside_size;
            }

            if (increase_size % DATA_SLICE_SIZE)
            {
                inc_slice_num = increase_size / DATA_SLICE_SIZE + 1;
            }
            else
            {
                inc_slice_num = increase_size / DATA_SLICE_SIZE;
            }

            for (i = 0; i < inc_slice_num; i++)
            {
                data_s = malloc(sizeof(data_slice));
                if (!data_s)
                {
                    printf("allocate memory failed! %s, %d\n", __func__, __LINE__);
                    goto err;
                }
                memset(data_s, 0, sizeof(data_slice));

                slist_init(&(data_s->list));

                data_s->data_node = memheap_alloc(&(ramfs->memheap), DATA_SLICE_SIZE);
                if (!data_s->data_node)
                {
                    free(data_s);
                    printf("allocate memory failed! %s, %d\n", __func__, __LINE__);
                    goto err;
                }
                memset(data_s->data_node, 0, DATA_SLICE_SIZE);

                if (i == 0)
                {
                    inc_add_root = data_s;
                }
                else
                {
                    slist_append(&(inc_add_root->list), &(data_s->list));
                }
            }
        }

        if (inc_add_root)
        {
            tail = slist_tail(&(dirent->data_slice_chain.list));
            tail->next = &(inc_add_root->list);
        }

        return 0;
err:
        if (inc_add_root)
        {
            ramfs_data_slice_free(dirent, inc_add_root);
        }
        return -1;
    }
    return 0;
}

static ssize_t ramfs_write(void *_ctx, int fd, const void *buf, size_t count)
{
    struct ramfs_dirent *dirent;
    struct ramfs *ramfs;

    ramfs = _ctx;
    RAMFS_ASSERT(ramfs != NULL);

    ramfs_file_t *file = ramfs_get_entry(_ctx, fd);

    ramfs_vfs_lock();

    dirent = (struct ramfs_dirent *)file->data;
    RAMFS_ASSERT(dirent != NULL);

    if (count + file->pos > file->size)
    {
        int increase_size = count + file->pos - file->size;

        if (ramfs_extend(file, increase_size))
        {
            ramfs_vfs_unlock();
            return 0;
        }
        dirent->size = file->pos + count;
        file->size = dirent->size;
    }

    if (count > 0)
    {
        int slice_pos = file->pos / DATA_SLICE_SIZE;
        int slice_inside_pos = file->pos % DATA_SLICE_SIZE;
        int size = count;
        int sz = 0;

        data_slice *data_s = slist_entry(dirent->data_slice_chain.list.next, data_slice, list);

        while (slice_pos-- > 0 && data_s != NULL)
        {
            if (data_s->list.next)
            {
                data_s = slist_entry(data_s->list.next, data_slice, list);
            }
            else
            {
                data_s = NULL;
            }
        }

        if (file->pos % DATA_SLICE_SIZE && data_s != NULL)
        {
            int len = MIN(DATA_SLICE_SIZE - slice_inside_pos, size);
            memcpy(data_s->data_node + slice_inside_pos, buf, len);
            size -= len;
            buf += len;
            sz += len;
            if (data_s->list.next)
            {
                data_s = slist_entry(data_s->list.next, data_slice, list);
            }
            else
            {
                data_s = NULL;
            }
        }

        while (size >= DATA_SLICE_SIZE && data_s != NULL)
        {
            memcpy(data_s->data_node, buf, DATA_SLICE_SIZE);
            size -= DATA_SLICE_SIZE;
            buf += DATA_SLICE_SIZE;
            sz += DATA_SLICE_SIZE;
            if (data_s->list.next)
            {
                data_s = slist_entry(data_s->list.next, data_slice, list);
            }
            else
            {
                data_s = NULL;
            }
        }

        if (size && data_s != NULL)
        {
            memcpy(data_s->data_node, buf, size);
            buf += size;
            sz += size;
            size = 0;
        }

        count = sz;
    }

    /* update file current position */
    file->pos += count;

    ramfs_vfs_unlock();
    return count;
}

static off_t ramfs_lseek(void *_ctx, int fd, off_t offset, int whence)
{
    ssize_t ret;
    struct ramfs *ctx = _ctx;
    ramfs_file_t *file = ramfs_get_entry(ctx, fd);

    switch (whence)
    {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            offset += file->pos;
            break;
        case SEEK_END:
            offset += file->size;
            break;
        default:
            break;
    }

    if (offset <= (off_t)file->size)
    {
        file->pos = offset;
        return file->pos;
    }
    else
    {
        struct ramfs_dirent *dirent;
        struct ramfs *ramfs;

        dirent = (struct ramfs_dirent *)file->data;
        RAMFS_ASSERT(dirent != NULL);

        ramfs = dirent->fs;
        RAMFS_ASSERT(ramfs != NULL);

        ramfs_vfs_lock();

        int increase_size = offset - file->size;
        if (file->size % DATA_SLICE_SIZE)
        {
            increase_size += (DATA_SLICE_SIZE  - file->size % DATA_SLICE_SIZE);
        }
        if (ramfs_extend(file, increase_size))
        {
            ramfs_vfs_unlock();
            return file->pos;
        }
        dirent->size = offset;
        file->size = dirent->size;

        file->pos = offset;
        ramfs_vfs_unlock();
        return file->pos;
    }
}

static int ramfs_close(void *_ctx, int fd)
{
    ramfs_vfs_lock();
    ramfs_file_t *file = ramfs_get_entry(_ctx, fd);

    ramfs_del_file(_ctx, fd);
    free(file);

    ramfs_vfs_unlock();
    return 0;
}

static int ramfs_closedir(void *_ctx, DIR *pdir)
{
    ssize_t ret;
    struct ramfs_dir *dir = (struct ramfs_dir *)pdir;

    ret = ramfs_close(_ctx, dir->index);
    if (!ret)
    {
#ifdef CONFIG_COMPONENTS_AMP
        amp_align_free(dir);
#else
        free(dir);
#endif
    }

    return ret;
}

static int ramfs_open(void *_ctx, const char *path, int flags, int mode)
{
    size_t size;
    struct ramfs *ramfs = _ctx;
    struct ramfs_dirent *dirent;
    ramfs_file_t *file;
    int index = 0;
    int ret = -1;

    RAMFS_ASSERT(ramfs != NULL);

    ramfs_vfs_lock();

    for (index = 0; index < MAX_OPENED_FILES; index++)
    {
        if (!ramfs_get_entry(_ctx, index))
        {
            break;
        }
    }

    if (index == MAX_OPENED_FILES)
    {
        printf("open too much files, limit %d\n", MAX_OPENED_FILES);
        goto err_unlock;
    }

    file = (ramfs_file_t *)malloc(sizeof(*file));
    if (!file)
    {
        goto err_unlock;
    }

    ramfs_vfs_unlock();

    if (flags & O_DIRECTORY)
    {
        if (flags & O_CREAT)
        {
            struct ramfs_dirent *parent;

            dirent = ramfs_lookup(ramfs, path, &size);
            if (dirent == &(ramfs->root)) /* it's root directory */
            {
                ret = -ENOENT;
                goto err_free;
            }

            if (dirent == NULL)
            {
                {
                    char *name_ptr, *ptr;

                    ramfs_vfs_lock();
                    /* create a file entry */
                    dirent = (struct ramfs_dirent *)
                             memheap_alloc(&(ramfs->memheap),
                                           sizeof(struct ramfs_dirent));
                    if (dirent == NULL)
                    {
                        ret = -ENOMEM;
                        goto err_free;
                    }

                    memset(dirent, 0, sizeof(struct ramfs_dirent));
                    /* remove '/' separator */
                    name_ptr = (char *)path;
                    while (*name_ptr == '/' && *name_ptr)
                    {
                        name_ptr ++;
                    }
                    ptr = strrchr(name_ptr, '/');
                    if (ptr)
                    {
                        name_ptr = ptr + 1;
                    }
                    strncpy(dirent->name, name_ptr, RAMFS_NAME_MAX);

                    INIT_LIST_HEAD(&(dirent->list));
                    INIT_LIST_HEAD(&(dirent->dirent_list));
                    memset(&(dirent->data_slice_chain), 0, sizeof(data_slice));
                    slist_init(&(dirent->data_slice_chain.list));
                    dirent->size = 0;
                    dirent->fs = ramfs;
                    dirent->type = RAMFS_DIR;

                    /* add to the root directory */
                    parent = ramfs_lookup_partent_dentry(ramfs, path);
                    if (parent)
                    {
                        list_add_tail(&(dirent->list), &parent->dirent_list);
                        dirent->parent = parent;
                    }
                    else
                    {
                        list_add_tail(&(dirent->list), &(ramfs->root.dirent_list));
                        dirent->parent = &(ramfs->root);
                    }
                    ramfs_vfs_unlock();
                }
            }
            else
            {
                ret = -ENOENT;
                goto err_free;
            }
        }

        /* open directory */
        dirent = ramfs_lookup(ramfs, path, &size);
        if (dirent == NULL)
        {
            ret = -ENOENT;
            goto err_free;
        }
        if (dirent == &(ramfs->root)) /* it's root directory */
        {
            if (!(flags & O_DIRECTORY))
            {
            }
        }
        if (dirent->type != RAMFS_DIR)
        {
            ret = -ENOENT;
            goto err_free;
        }
    }
    else
    {
        dirent = ramfs_lookup(ramfs, path, &size);
        if (dirent == &(ramfs->root)) /* it's root directory */
        {
            ret = -ENOENT;
            goto err_free;
        }

        if (dirent == NULL)
        {
            if (flags & O_CREAT || flags & O_WRONLY)
            {
                char *name_ptr;

                /* create a file entry */
                dirent = (struct ramfs_dirent *)
                         memheap_alloc(&(ramfs->memheap),
                                       sizeof(struct ramfs_dirent));
                if (dirent == NULL)
                {
                    ret = -ENOMEM;
                    goto err_free;
                }
                memset(dirent, 0, sizeof(struct ramfs_dirent));

                ramfs_vfs_lock();

                /* remove '/' separator */
                name_ptr = (char *)path;
                while (*name_ptr == '/' && *name_ptr)
                {
                    name_ptr ++;
                }
                {
                    char *ptr = strrchr(name_ptr, '/');
                    if (ptr)
                    {
                        name_ptr = ptr + 1;
                    }
                }
                strncpy(dirent->name, name_ptr, RAMFS_NAME_MAX);

                INIT_LIST_HEAD(&(dirent->list));
                memset(&(dirent->data_slice_chain), 0, sizeof(data_slice));
                slist_init(&(dirent->data_slice_chain.list));
                dirent->size = 0;
                dirent->fs = ramfs;
                dirent->type = RAMFS_FILE;
                INIT_LIST_HEAD(&(dirent->dirent_list));

                struct ramfs_dirent *parent;
                parent = ramfs_lookup_partent_dentry(ramfs, path);
                if (parent)
                {
                    list_add_tail(&(dirent->list), &parent->dirent_list);
                    dirent->parent = parent;
                }
                else
                {
                    list_add_tail(&(dirent->list), &(ramfs->root.dirent_list));
                    dirent->parent = &(ramfs->root);
                }
                ramfs_vfs_unlock();
            }
            else
            {
                ret = -ENOENT;
                goto err_free;
            }
        }

        /* Creates a new file.
         * If the file is existing, it is truncated and overwritten.
         */
        if (flags & O_TRUNC)
        {
            dirent->size = 0;

            ramfs_vfs_lock();

            if (dirent->data_slice_chain.list.next)
            {
                ramfs_data_slice_free(dirent, list_entry(dirent->data_slice_chain.list.next, data_slice, list));
            }
            memset(&(dirent->data_slice_chain), 0, sizeof(data_slice));
            slist_init(&(dirent->data_slice_chain.list));
            ramfs_vfs_unlock();
        }
    }

    ramfs_vfs_lock();

    file->data = dirent;
    file->size = dirent->size;
    if (flags & O_APPEND)
    {
        file->pos = file->size;
    }
    else
    {
        file->pos = 0;
    }
    ramfs_vfs_unlock();

    ramfs_add_file(ramfs, index, file, path);
    return index;

err_free:
    free(file);

err_unlock:
    ramfs_vfs_unlock();
    return ret;
}


static DIR *ramfs_opendir(void *_ctx, const char *path)
{
    struct ramfs_dir *dir;

#ifdef CONFIG_COMPONENTS_AMP
    dir = amp_align_malloc(sizeof(*dir));
#else
    dir = malloc(sizeof(*dir));
#endif
    if (!dir)
    {
        return NULL;
    }
    memset(dir, 0, sizeof(*dir));

    int index = ramfs_open(_ctx, path, O_DIRECTORY, 0);
    if (index >= 0)
    {
        dir->index = index;
        return (DIR *)dir;
    }
#ifdef CONFIG_COMPONENTS_AMP
    amp_align_free(dir);
#else
    free(dir);
#endif
    return NULL;
}

static int ramfs_mkdir(void *_ctx, const char *name, mode_t mode)
{
    int ret = -1;
    int index = ramfs_open(_ctx, name, O_DIRECTORY | O_CREAT, 0);
    if (index >= 0)
    {
        ramfs_close(_ctx, index);
        ret = 0;
    }
    return ret;
}

static int ramfs_stat(void *_ctx, const char *path, struct stat *st)
{
    size_t size;
    struct ramfs_dirent *dirent;
    struct ramfs *ramfs;

    ramfs = (struct ramfs *)_ctx;
    dirent = ramfs_lookup(ramfs, path, &size);

    if (dirent == NULL)
    {
        return -ENOENT;
    }

    ramfs_vfs_lock();

    st->st_dev = 0;
    st->st_mode = dirent->type == RAMFS_FILE ? S_IFREG : S_IFDIR;
    st->st_mode |= S_IRWXU | S_IRWXG | S_IRWXO;

    st->st_size = dirent->size;
    st->st_mtime = 0;

    ramfs_vfs_unlock();
    return 0;
}

static int ramfs_fstat(void *ctx, int fd, struct stat *st)
{
    return ramfs_stat(ctx, ramfs_get_path(ctx, fd), st);
}

int ramfs_getdents(ramfs_file_t *file,
                   struct dirent *dirp,
                   uint32_t count)
{
    size_t index, end;
    struct dirent *d;
    struct ramfs_dirent *dirent;
    struct ramfs_dirent *target_dirent;
    struct ramfs *ramfs;

    dirent = (struct ramfs_dirent *)file->data;
    target_dirent = dirent;

    ramfs  = dirent->fs;
    RAMFS_ASSERT(ramfs != NULL);

    /* make integer count */
    count = (count / sizeof(struct dirent));
    if (count == 0)
    {
        return -EINVAL;
    }

    ramfs_vfs_lock();

    end = file->pos + count;
    index = 0;
    count = 0;

    if (target_dirent->dirent_list.next == &(target_dirent->dirent_list))
    {
        ramfs_vfs_unlock();
        return count * sizeof(struct dirent);
    }

    for (dirent = list_entry(target_dirent->dirent_list.next, struct ramfs_dirent, list);
         dirent->list.next != &(target_dirent->dirent_list) && index < end;
         dirent = list_entry(dirent->list.next, struct ramfs_dirent, list))
    {
        if (index >= (size_t)file->pos)
        {
            d = dirp + count;
            d->d_type = dirent->type == RAMFS_FILE ? DT_REG : DT_DIR;
            strncpy(d->d_name, dirent->name, RAMFS_NAME_MAX);

            count += 1;
            file->pos += 1;
        }
        index += 1;
    }

    if (dirent->list.next == &(target_dirent->dirent_list) && index < end)
    {
        if (index >= (size_t)file->pos)
        {
            d = dirp + count;
            d->d_type = dirent->type == RAMFS_FILE ? DT_REG : DT_DIR;
            strncpy(d->d_name, dirent->name, RAMFS_NAME_MAX);

            count += 1;
            file->pos += 1;
        }
        index += 1;
    }
    ramfs_vfs_unlock();
    return count * sizeof(struct dirent);
}

static int ramfs_unlink(void *_ctx, const char *path)
{
    size_t size;
    struct ramfs *ramfs;
    struct ramfs_dirent *dirent;

    ramfs = (struct ramfs *)_ctx;
    RAMFS_ASSERT(ramfs != NULL);

    dirent = ramfs_lookup(ramfs, path, &size);
    if (dirent == NULL)
    {
        return -ENOENT;
    }

    if (&(ramfs->root) == dirent)
    {
        return -EIO;
    }

    if (dirent->type == RAMFS_DIR)
    {
        if (!list_empty(&dirent->dirent_list))
        {
            printf("Can not unlink non empty directory!\n");
            return -EIO;
        }
    }

    ramfs_vfs_lock();

    list_del(&(dirent->list));
    if (dirent->data_slice_chain.list.next)
    {
        ramfs_data_slice_free(dirent, slist_entry(dirent->data_slice_chain.list.next, data_slice, list));
    }
    memheap_free(dirent);

    ramfs_vfs_unlock();
    return 0;
}

static int ramfs_rename(void *_ctx, const char *oldpath, const char *newpath)
{
    struct ramfs_dirent *dirent;
    struct ramfs *ramfs;
    size_t size;

    ramfs = (struct ramfs *)_ctx;
    RAMFS_ASSERT(ramfs != NULL);

    dirent = ramfs_lookup(ramfs, newpath, &size);
    if (dirent != NULL)
    {
        return -EEXIST;
    }

    dirent = ramfs_lookup(ramfs, oldpath, &size);
    if (dirent == NULL)
    {
        return -ENOENT;
    }

    ramfs_vfs_lock();

    const char *subpath;
    char *ptr;

    subpath = newpath;
    while (*subpath == '/' && *subpath)
    {
        subpath ++;
    }

    if (! *subpath)
    {
        ramfs_vfs_unlock();
        return -EINVAL;
    }

    ptr = strrchr(subpath, '/');
    if (ptr)
    {
        subpath = ptr + 1;
    }
    strncpy(dirent->name, subpath, RAMFS_NAME_MAX);

    {
        struct ramfs_dirent *new_parent;
        struct ramfs_dirent *old_parent;

        old_parent = dirent->parent;
        list_del(&(dirent)->list);

        /* add to the root directory */
        new_parent = ramfs_lookup_partent_dentry(ramfs, newpath);
        if (new_parent)
        {
            list_add_tail(&(dirent->list), &new_parent->dirent_list);
            dirent->parent = new_parent;
        }
        else
        {
            list_add_tail(&(dirent->list), &(ramfs->root.dirent_list));
            dirent->parent = &(ramfs->root);
        }
    }
    ramfs_vfs_unlock();
    return 0;
}

static int ramfs_ftruncate(ramfs_file_t *file, off_t length)
{
    int ret = -1;

    if (file->flags & O_DIRECTORY)
    {
        return -EBADF;
    }
    else
    {
        struct ramfs_dirent *dirent;
        struct ramfs *ramfs;

        ramfs_vfs_lock();

        dirent = (struct ramfs_dirent *)file->data;
        RAMFS_ASSERT(dirent != NULL);

        ramfs = dirent->fs;
        RAMFS_ASSERT(ramfs != NULL);

        if (length == file->size)
        {
            ramfs_vfs_unlock();
            return 0;
        }
        else if (length > file->size)
        {
            int increase_size = length - file->size;

            if (ramfs_extend(file, increase_size))
            {
                ramfs_vfs_unlock();
                return -1;
            }

            dirent->size = length;
            file->size = dirent->size;
            if (file->size <= file->pos)
            {
                file->pos = file->size - 1;
            }
            ramfs_vfs_unlock();
            return 0;
        }
        else
        {
            data_slice *data_s = NULL, *last_data_s = NULL;
            int slice_pos = length / DATA_SLICE_SIZE;
            int slice_inside_pos = length % DATA_SLICE_SIZE;

            if (slice_inside_pos != 0)
            {
                slice_pos ++;
            }

            data_s = slist_entry(dirent->data_slice_chain.list.next, data_slice, list);
            last_data_s = data_s;

            while (slice_pos-- > 0 && data_s != NULL)
            {
                if (data_s->list.next)
                {
                    data_s = slist_entry(data_s->list.next, data_slice, list);
                }
                else
                {
                    data_s = NULL;
                }

                if (slice_pos == 1)
                {
                    last_data_s = data_s;
                }
            }

            if (data_s)
            {
                if (last_data_s)
                {
                    last_data_s->list.next = NULL;
                }
                ramfs_data_slice_free(dirent, data_s);
            }

            dirent->size = length;
            file->size = dirent->size;
            if (file->size <= file->pos)
            {
                file->pos = file->size - 1;
            }
            ramfs_vfs_unlock();
            return 0;
        }
    }
}

struct ramfs *ramfs_create(uint8_t *pool, size_t size)
{
    struct ramfs *ramfs;
    uint8_t *data_ptr;
    int result;

    size  = RAMFS_ALIGN_DOWN(size, RAMFS_ALIGN_SIZE);
    ramfs = (struct ramfs *)pool;

    memset(ramfs, 0, sizeof(*ramfs));

    data_ptr = (uint8_t *)(ramfs + 1);
    size = size - sizeof(struct ramfs);
    size = RAMFS_ALIGN_DOWN(size, RAMFS_ALIGN_SIZE);

    result = memheap_init(&ramfs->memheap, "ramfs", data_ptr, size);
    if (result != 0)
    {
        return NULL;
    }

    /* initialize ramfs object */
    ramfs->magic = RAMFS_MAGIC;

    /* initialize root directory */
    memset(&(ramfs->root), 0x00, sizeof(ramfs->root));
    INIT_LIST_HEAD(&(ramfs->root.list));
    ramfs->root.size = 0;
    strcpy(ramfs->root.name, ".");
    ramfs->root.fs = ramfs;
    slist_init(&(ramfs->root.data_slice_chain.list));
    ramfs->root.type = RAMFS_DIR;

    INIT_LIST_HEAD(&(ramfs->root.dirent_list));
    ramfs_vfs_init();

    return ramfs;
}


static struct dirent *ramfs_readdir(void *_ctx, DIR *pdir)
{
    ssize_t ret;
    struct ramfs *ctx = _ctx;
    struct ramfs_dir *dir = (struct ramfs_dir *)pdir;
    ramfs_file_t *file = ramfs_get_entry(ctx, dir->index);

    memset(&dir->e, 0, sizeof(struct dirent));

    ret = ramfs_getdents(file, &dir->e, sizeof(struct dirent));
    if (ret <= 0)
    {
        return NULL;
    }

    return &dir->e;
}

static int ramfs_link(void *ctx, const char *n1, const char *n2)
{
    return -ENOSYS;
}

static int ramfs_readdir_r(void *_ctx, DIR *pdir, struct dirent *entry,
                           struct dirent **out_dirent)
{
    return -ENOSYS;
}

static void ramfs_seekdir(void *_ctx, DIR *pdir, long offset)
{
}

static long ramfs_telldir(void *_ctx, DIR *pdir)
{
    return 0;
}

static int ramfs_rmdir(void *_ctx, const char *name)
{
    return ramfs_unlink(_ctx, name);
}

static int ramfs_access(void *_ctx, const char *path, int amode)
{
    struct stat st;
    int ret;

    ret = ramfs_stat(_ctx, path, &st);
    if (ret < 0)
    {
        return ret;
    }

    switch (amode)
    {
        case F_OK:
            return 0;
        case R_OK:
            return st.st_mode & S_IRUSR ? 0 : 1;
        case W_OK:
            return st.st_mode & S_IWUSR ? 0 : 1;
        case X_OK:
            return st.st_mode & S_IXUSR ? 0 : 1;
    }
}

static int ramfs_fsync(void *_ctx, int fd)
{
    return 0;
}

int ramfs_register_vfs(char *path, struct ramfs *ramfs)
{
    const vfs_t vfs =
    {
        .flags = VFS_FLAG_CONTEXT_PTR,
        .open_p = ramfs_open,
        .write_p = &ramfs_write,
        .read_p = &ramfs_read,
        .close_p = &ramfs_close,
        .lseek_p = &ramfs_lseek,
        .fstat_p = &ramfs_fstat,
        .stat_p = &ramfs_stat,
        .link_p = &ramfs_link,
        .unlink_p = &ramfs_unlink,
        .rename_p = &ramfs_rename,
        .opendir_p = &ramfs_opendir,
        .closedir_p = &ramfs_closedir,
        .readdir_p = &ramfs_readdir,
        .readdir_r_p = &ramfs_readdir_r,
        .seekdir_p = &ramfs_seekdir,
        .telldir_p = &ramfs_telldir,
        .mkdir_p = &ramfs_mkdir,
        .rmdir_p = &ramfs_rmdir,
        .statfs_p = &ramfs_statfs,
        .fstatfs_p = &ramfs_fstatfs,
        .access_p = &ramfs_access,
        .fsync_p = &ramfs_fsync,
    };

    return vfs_register(path, &vfs, ramfs);
}

int ramfs_umount(char *path)
{
    return vfs_unregister(path);
}

int ramfs_create_mount(char *path, size_t size)
{
    struct ramfs *ramfs;
    int ret = -1;

    if (ramfs_pool)
    {
        printf("ramfs have mount!\n");
        return ret;
    }

    ramfs_pool = malloc(size);

    if (ramfs_pool)
    {
        ramfs = (struct ramfs *) ramfs_create((uint8_t *)ramfs_pool, size);
        if (ramfs != NULL)
        {
            if (ramfs_register_vfs(path, ramfs))
            {
                printf("Mount RAMDisk failed!\n");
                free(ramfs_pool);
                ramfs_pool = NULL;
            }
            else
            {
                ret = 0;
            }
        }
    }
    else
    {
        printf("alloc ramfs pool failed\n");
    }
    return ret;
}

