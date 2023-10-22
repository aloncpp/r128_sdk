/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vfs.h"
#include "ExampleCustomerWriter.h"

struct ExampleCustomerWriterImpl
{
    CdxWriterT base;
    vfs_file_t *vfs;
};

static int __CdxExampleConnect(CdxWriterT *writer)
{
    struct ExampleCustomerWriterImpl *impl;

    impl = (struct ExampleCustomerWriterImpl *)writer;

    vfs_unlink("data/record/2.amr");
    impl->vfs = vfs_open("data/record/2.amr", VFS_RDWR | VFS_CREAT);
    if (impl->vfs == NULL) {
        return -1;
    }

    return 0;
}

static int __CdxExampleRead(CdxWriterT *writer, void *buf, int size)
{
    return 0;
}

static int __CdxExampleWrite(CdxWriterT *writer, void *buf, int size)
{
    uint32_t write_len;
    struct ExampleCustomerWriterImpl *impl;

    impl = (struct ExampleCustomerWriterImpl *)writer;

    write_len = vfs_write(impl->vfs, buf, size);

    return write_len;
}

static long __CdxExampleSeek(CdxWriterT *writer, long moffset, int mwhere)
{
    return 0;
}

static long __CdxExampleTell(CdxWriterT *writer)
{
    return 0;
}

static int __CdxExampleClose(CdxWriterT *writer)
{
    struct ExampleCustomerWriterImpl *impl;

    impl = (struct ExampleCustomerWriterImpl *)writer;

    vfs_close(impl->vfs);
    free(impl);

    return 0;
}

static const struct CdxWriterOps exampleCustomerWriteOps =
{
    .cdxConnect   =  __CdxExampleConnect,
    .cdxRead      =  __CdxExampleRead,
    .cdxWrite     =  __CdxExampleWrite,
    .cdxSeek      =  __CdxExampleSeek,
    .cdxTell      =  __CdxExampleTell,
    .cdxClose     =  __CdxExampleClose
};

CdxWriterT *ExampleCustomerWriterCreat()
{
    struct ExampleCustomerWriterImpl *impl;

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        printf("example customer writer create fail.\n");
        return NULL;
    }

    memset(impl, 0, sizeof(*impl));

    impl->base.ops = &exampleCustomerWriteOps;

    return &impl->base;
}

