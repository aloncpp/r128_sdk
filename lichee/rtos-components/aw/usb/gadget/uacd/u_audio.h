/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PART'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNER'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PART'S TECHNOLOGY.


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

#ifndef __U_AUDIO_H__
#define __U_AUDIO_H__


#define UACD_VERSION 	("AW-V0.5")


#define UACD_VERBOSE_MASK 	(1 << 2)
#define UACD_DEBUG_MASK 	(1 << 1)
#define UACD_INFO_MASK 	 	(1 << 0)

#ifndef unlikely
#define unlikely(x)             __builtin_expect ((x), 0)
#endif

#define LOG_COLOR_NONE		"\e[0m"
#define LOG_COLOR_GREEN		"\e[32m"
#define LOG_COLOR_BLUE		"\e[34m"
#define LOG_COLOR_RED		"\e[31m"

extern int8_t g_uacd_debug_mask;
#define uacd_info(fmt, args...) \
	do { \
		if (unlikely(g_uacd_debug_mask & UACD_INFO_MASK)) \
			printf(LOG_COLOR_BLUE"[UACD-INFO][%s] line:%d " fmt "\n" LOG_COLOR_NONE, \
					__func__, __LINE__, ##args); \
	} while (0)

#define uacd_debug(fmt, args...) \
	do { \
		if (unlikely(g_uacd_debug_mask & UACD_DEBUG_MASK)) \
			printf(LOG_COLOR_GREEN"[UACD-DEBUG][%s] line:%d " fmt "\n" LOG_COLOR_NONE, \
					__func__, __LINE__, ##args); \
	} while (0)

#define uacd_verbose(fmt, args...) \
	do { \
		if (unlikely(g_uacd_debug_mask & UACD_VERBOSE_MASK)) \
			printf("[UACD-VERBOSE][%s] line:%d " fmt "\n", \
					__func__, __LINE__, ##args); \
	} while (0)

#define uacd_err(fmt, args...) \
	do { \
		printf(LOG_COLOR_RED"[UACD-ERR][%s] line:%d " fmt "\n" LOG_COLOR_NONE, \
				__func__, __LINE__, ##args); \
	} while (0)


#endif /*__U_AUDIO_H__*/
