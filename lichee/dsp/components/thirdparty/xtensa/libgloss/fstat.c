/* fstat.c  -  get status of a file */

/*
 * Copyright (c) 2011 Tensilica Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <sys/stat.h>
#include "gloss.h"


/*
 *  fstat
 *
 *  Return something sensible for fd 0 (stdin) or 1 (stdout).
 *
 *  Nominally, struct stat contains this:
 *
 *	struct stat {
 *	    dev_t         st_dev;      // device
 *	    ino_t         st_ino;      // inode
 *	    mode_t        st_mode;     // protection
 *	    nlink_t       st_nlink;    // number of hard links
 *	    uid_t         st_uid;      // user ID of owner
 *	    gid_t         st_gid;      // group ID of owner
 *	    dev_t         st_rdev;     // device type (if inode device)
 *	    off_t         st_size;     // total size, in bytes
 *	    blksize_t     st_blksize;  // blocksize for filesystem I/O
 *	    blkcnt_t      st_blocks;   // number of blocks allocated
 *	    time_t        st_atime;    // time of last access
 *	    time_t        st_mtime;    // time of last modification
 *	    time_t        st_ctime;    // time of last status change
 *	};
 */

int
_FUNC (fstat, int fd, struct stat *buf)
{
    buf->st_uid = 0;		/* owned by root */
    buf->st_mode = S_IFCHR;	/* as for a char device (eg. TTY) */
    buf->st_blksize = 0;
    buf->st_size = 0;

    /* NOTE: other fields left uninitialized! */
    /*buf->st_dev = 0;*/
    /*buf->st_ino = 0;*/
    /*buf->st_nlink = 1;*/
    /*buf->st_gid = 0;*/
    /*buf->st_rdev = 0;*/
    /*buf->st_blocks = 0;*/
    /*buf->st_atime = 0;*/
    /*buf->st_mtime = 0;*/
    /*buf->st_ctime = 0;*/

    return 0;
}

