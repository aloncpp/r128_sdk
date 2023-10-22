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
#ifndef __SND_SUNXI_ALG_CFG_H
#define __SND_SUNXI_ALG_CFG_H

#define ALG_CFG_PARM_LEN_MIN	14
#define ALG_CFG_PARM_LEN_MAX	24

enum SUNXI_ALG_CFG_DOMAIN {
	SUNXI_ALG_CFG_DOMAIN_MEQ = 0,
	SUNXI_ALG_CFG_DOMAIN_PEQ,
	SUNXI_ALG_CFG_DOMAIN_1BDRC,
	SUNXI_ALG_CFG_DOMAIN_3BDRC,
	SUNXI_ALG_CFG_DOMAIN_HPF,
};

struct alg_cfg_reg_domain {
	void *reg_base;
	unsigned int reg_min;
	unsigned int reg_max;
};

#if CONFIG_SND_COMMON_SUNXI_ALG_CFG
extern int sunxi_alg_cfg_domain_set(struct alg_cfg_reg_domain *domain,
				    enum SUNXI_ALG_CFG_DOMAIN domain_sel);
extern int sunxi_alg_cfg_domain_get(struct alg_cfg_reg_domain **domain,
				    enum SUNXI_ALG_CFG_DOMAIN domain_sel);
void sunxi_alg_cfg_set(unsigned int reg, unsigned int reg_val,
			      struct alg_cfg_reg_domain *domain);

#else
static inline int sunxi_alg_cfg_domain_set(struct alg_cfg_reg_domain *domain,
					   enum SUNXI_ALG_CFG_DOMAIN domain_sel)
{
	return 0;
}
static inline int sunxi_alg_cfg_domain_get(struct alg_cfg_reg_domain **domain,
				    enum SUNXI_ALG_CFG_DOMAIN domain_sel)
{
	return 0;
}
static inline void sunxi_alg_cfg_set(unsigned int reg, unsigned int reg_val,
				struct alg_cfg_reg_domain *domain)
{
}
#endif

#endif /* __SND_SUNXI_ALG_CFG_H */
