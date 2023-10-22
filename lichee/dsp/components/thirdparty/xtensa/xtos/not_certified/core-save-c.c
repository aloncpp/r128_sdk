
/*
 * core-save-c.c -- C wrapper for IDMA function(s) called from PSO routines.
 */

/*
 * Copyright (c) 1999-2018 Cadence Design Systems, Inc.
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

#include <xtensa/xtruntime-core-state.h>
#include "xtos-internal.h"

#if XCHAL_HAVE_IDMA
# include <xtensa/idma.h>
void idma_pso_save(uint32_t flags, uint32_t* mem) __attribute__((weak));
#endif

void xtos_C_core_save (uint32_t flags, XtosCoreState* saveArea);

void xtos_C_core_save (uint32_t flags, XtosCoreState* saveArea)
{
#if XCHAL_HAVE_IDMA
  if (idma_pso_save) {
    idma_pso_save(flags, (uint32_t *)(saveArea->idmaregs));
  }
#else
  UNUSED(flags);
  UNUSED(saveArea);
#endif
}

