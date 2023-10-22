/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __AUDIO_HW_H
#define __AUDIO_HW_H

#include <assert.h>
#include <aw_common.h>
#include <aw_list.h>
#include <errno.h>
#include <stdbool.h>

#include "AudioBase.h"

#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/common.h>


typedef struct audio_hw audio_hw_t;


void add_default_ahw(void);

int audio_hw_open(audio_hw_t *ahw);
int audio_hw_close(audio_hw_t *ahw);

audio_hw_t *audio_hw_init(const char *name, int8_t stream);
int audio_hw_destroy(audio_hw_t *ahw);
int audio_hw_write(audio_hw_t *ahw, struct rb_attr *rb);
int audio_hw_read(audio_hw_t *ahw, struct rb_attr *rb);


int audio_hw_time_to_frames(audio_hw_t *ahw, uint32_t ms);
as_pcm_config_t *audio_hw_pcm_config(audio_hw_t *ahw);
uint8_t audio_hw_stream(audio_hw_t *ahw);
struct audio_hw_elem *audio_hw_elem_item(audio_hw_t *ahw);
int audio_hw_instance(audio_hw_t *ahw);
const char *audio_hw_name(audio_hw_t *ahw);
int audio_hw_name_to_instance(const char *name, int8_t stream);
void audio_hw_set_private_data(audio_hw_t *ahw, void *pdata);
void *audio_hw_get_private_data(audio_hw_t *ahw);


#endif /* __AUDIO_HW_H */
