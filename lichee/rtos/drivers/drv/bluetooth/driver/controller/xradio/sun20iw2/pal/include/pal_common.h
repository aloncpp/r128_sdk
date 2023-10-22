/*****************************************************************************/
/*!
 *  \file
 *
 *  \brief      pal common file.
 *
 *  Copyright (c) 2019-2020 Xradio, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*****************************************************************************/

#ifndef __PAL_COMMON_H_
#define __PAL_COMMON_H_

/** Includes --------------------------------------------------------------- */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/** Types ------------------------------------------------------------------ */
/** Constants -------------------------------------------------------------- */
/** Macro ------------------------------------------------------------------ */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef RANGEOF
#define RANGEOF(num, start, end) (((num) <= (end)) && ((num) >= (start)))
#endif

/** Functions -------------------------------------------------------------- */

#endif /* __PAL_COMMON_H_ */

/************** (C) COPYRIGHT 2019-2020 XRADIO, INC. ******* END OF FILE *****/

