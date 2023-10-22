/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
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


#include <stdio.h>
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "arch/sys_arch.h"
#include "lwip/stats.h"
#include <stdlib.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include "console.h"

#define AW_NUM_TCP_PCB_LISTS               4
extern struct tcp_pcb ** const tcp_pcb_lists[AW_NUM_TCP_PCB_LISTS];

static const char* tcp_state_to_str(enum tcp_state state)
{
	switch(state) {
		case CLOSED:
			return "CLOSED";
		case LISTEN:
			return "LISTEN";
		case SYN_SENT:
			return "SYN_SENT";
		case SYN_RCVD:
			return "SYN_RCVD";
		case ESTABLISHED:
			return "ESTABLISHED";
		case FIN_WAIT_1:
			return "FIN_WAIT_1";
		case FIN_WAIT_2:
			return "FIN_WAIT_2";
		case CLOSE_WAIT:
			return "CLOSE_WAIT";
		case CLOSING:
			return "CLOSING";
		case LAST_ACK:
			return "LAST_ACK";
		case TIME_WAIT:
			return "TIME_WAIT";
		default:
			return "ERROR STATE";
	}
}

static void tcp_netstat(void)
{
	int i;
	struct tcp_pcb *cpcb;

	for(i = 0; i<AW_NUM_TCP_PCB_LISTS; i++) {
		for(cpcb = *tcp_pcb_lists[i];cpcb != NULL ;cpcb = cpcb->next) {
			printf("%s\t%-3"U16_F".%-3"U16_F".%-3"U16_F".%-3"U16_F":%d\t\
%-3"U16_F".%-3"U16_F".%-3"U16_F".%-3"U16_F":%d\t%s\n","tcp",
		ip4_addr1_16(&cpcb->local_ip),ip4_addr2_16(&cpcb->local_ip),
		ip4_addr3_16(&cpcb->local_ip),ip4_addr4_16(&cpcb->local_ip),
		cpcb->local_port,
		ip4_addr1_16(&cpcb->remote_ip),ip4_addr2_16(&cpcb->remote_ip),
		ip4_addr3_16(&cpcb->remote_ip),ip4_addr4_16(&cpcb->remote_ip),
		cpcb->remote_port,
		tcp_state_to_str(cpcb->state));
		}
	}
}
extern struct udp_pcb *udp_pcbs;

static void udp_netstat(void)
{
	struct udp_pcb *pcb;
	for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
	printf("%s\t%-3"U16_F".%-3"U16_F".%-3"U16_F".%-3"U16_F":%d\t\
%-3"U16_F".%-3"U16_F".%-3"U16_F".%-3"U16_F":%d\n","udp",
		ip4_addr1_16(&pcb->local_ip),ip4_addr2_16(&pcb->local_ip),
		ip4_addr3_16(&pcb->local_ip),ip4_addr4_16(&pcb->local_ip),
		pcb->local_port,
		ip4_addr1_16(&pcb->remote_ip),ip4_addr2_16(&pcb->remote_ip),
		ip4_addr3_16(&pcb->remote_ip),ip4_addr4_16(&pcb->remote_ip),
		pcb->remote_port);
	}
}

void netstat(void)
{
	printf("proto\tLocal Address\t\tRemote Address\t\tState\n");
	tcp_netstat();
	udp_netstat();
}

FINSH_FUNCTION_EXPORT_CMD(netstat,netstat, Console net status Command);

struct count {
	unsigned int a;
	unsigned int b;
};

struct NetDataStatistics{
	struct count drv_tx;
	struct count drv_rx;
	struct count ip_tx;
	struct count ip_rx;
	struct count tcp_tx;
	struct count tcp_rx;
	struct count udp_tx;
	struct count udp_rx;
};

struct NetDataStatistics *dsptr = NULL;

#define THRESHOLD 3000000000

void aw_dbg_drv_tx_data_len(unsigned int len)
{
	if(dsptr == NULL) {
		return ;
	}

	dsptr->drv_tx.a += len;

	if(dsptr->drv_tx.a > THRESHOLD) {
		dsptr->drv_tx.b ++;
		dsptr->drv_tx.a -= THRESHOLD;
	}
}

void aw_dbg_drv_rx_data_len(unsigned int len)
{
	if(dsptr == NULL) {
		return ;
	}

	dsptr->drv_rx.a += len;

	if(dsptr->drv_rx.a > THRESHOLD) {
		dsptr->drv_rx.b ++;
		dsptr->drv_rx.a -= THRESHOLD;
	}
}

void aw_dbg_ip_tx_data_len(unsigned int len)
{
	if(dsptr == NULL) {
		return ;
	}

	dsptr->ip_tx.a += len;

	if(dsptr->ip_tx.a > THRESHOLD) {
		dsptr->ip_tx.b ++;
		dsptr->ip_tx.a -= THRESHOLD;
	}
}

void aw_dbg_ip_rx_data_len(unsigned int len)
{
	if(dsptr == NULL) {
		return ;
	}

	dsptr->ip_rx.a += len;

	if(dsptr->ip_rx.a > THRESHOLD) {
		dsptr->ip_rx.b ++;
		dsptr->ip_rx.a -= THRESHOLD;
	}
}

void aw_dbg_tcp_tx_data_len(unsigned int len)
{
	if(dsptr == NULL) {
		return ;
	}

	dsptr->tcp_tx.a += len;

	if(dsptr->tcp_tx.a > THRESHOLD) {
		dsptr->tcp_tx.b ++;
		dsptr->tcp_tx.a -= THRESHOLD;
	}
}

void aw_dbg_tcp_rx_data_len(unsigned int len)
{
	if(dsptr == NULL) {
		return ;
	}

	dsptr->tcp_rx.a += len;

	if(dsptr->tcp_rx.a > THRESHOLD) {
		dsptr->tcp_rx.b ++;
		dsptr->tcp_rx.a -= THRESHOLD;
	}
}

void aw_dbg_udp_tx_data_len(unsigned int len)
{
	if(dsptr == NULL) {
		return ;
	}

	dsptr->udp_tx.a += len;

	if(dsptr->udp_tx.a > THRESHOLD) {
		dsptr->udp_tx.b ++;
		dsptr->udp_tx.a -= THRESHOLD;
	}
}

void aw_dbg_udp_rx_data_len(unsigned int len)
{
	if(dsptr == NULL) {
		return ;
	}

	dsptr->udp_rx.a += len;

	if(dsptr->udp_rx.a > THRESHOLD) {
		dsptr->udp_rx.b ++;
		dsptr->udp_rx.a -= THRESHOLD;
	}
}

void aw_dbg_net_info(void)
{
	if(dsptr == NULL) {
		dsptr = (struct NetDataStatistics*)calloc(1,sizeof(struct NetDataStatistics));
		if(dsptr == NULL) {
			printf("data resource malloc failed.\n");
			return ;
		}
	}
	printf("Calculation-Example: a:b total=a*3000000000+b");
	printf("\r\nLayer\t\tTX(Byte)\t\t\tRX(Byte)\r\n");
	printf("driver: %8u:%-8u\t\t%8u:%-8u\n",dsptr->drv_tx.b,dsptr->drv_tx.a,
			dsptr->drv_rx.b,dsptr->drv_rx.a);
	printf("ip    : %8u:%-8u\t\t%8u:%-8u\n",dsptr->ip_tx.b,dsptr->ip_tx.a,
			dsptr->ip_rx.b,dsptr->ip_rx.a);
	printf("tcp   : %8u:%-8u\t\t%8u:%-8u\n",dsptr->tcp_tx.b,dsptr->tcp_tx.a,
			dsptr->tcp_rx.b,dsptr->tcp_rx.a);
	printf("udp   : %8u:%-8u\t\t%8u:%-8u\n",dsptr->udp_tx.b,dsptr->udp_tx.a,
			dsptr->udp_rx.b,dsptr->udp_rx.a);
}
FINSH_FUNCTION_EXPORT_CMD(aw_dbg_net_info,netinfo, Console net info Command);

void aw_lwip_stat(void)
{
	stats_display();
}

FINSH_FUNCTION_EXPORT_CMD(aw_lwip_stat,lwipstat, Console lwipstat Command);
