#include "lwip/opt.h"

#if LWIP_STATS /* don't build if not configured for use in lwipopts.h */

#include "lwip/def.h"
#include "lwip/stats.h"
#include "lwip/mem.h"
#include "lwip/debug.h"

#include <string.h>

struct stats_ lwip_stats;

void
stats_init(void)
{
#ifdef LWIP_DEBUG
#if MEM_STATS
  lwip_stats.mem.name = "MEM";
#endif /* MEM_STATS */
#endif /* LWIP_DEBUG */
}

#if LWIP_STATS_DISPLAY
void stats_display_proto(struct stats_proto *proto, const char *name)
{
	printf("%-8s%8"STAT_COUNTER_F"%8"STAT_COUNTER_F"%8"STAT_COUNTER_F"%8"STAT_COUNTER_F"\
%8"STAT_COUNTER_F"%8"STAT_COUNTER_F"%8"STAT_COUNTER_F"%8"STAT_COUNTER_F"\
%8"STAT_COUNTER_F"%8"STAT_COUNTER_F"%8"STAT_COUNTER_F"%8"STAT_COUNTER_F"\n",name,
			proto->xmit,proto->recv,proto->fw,proto->drop,proto->chkerr,proto->lenerr,
			proto->memerr,proto->rterr,proto->proterr,proto->opterr,proto->err,
			proto->cachehit);
}

#if MEM_STATS || MEMP_STATS
void stats_display_mem(struct stats_mem *mem, const char *name)
{
  printf("%-16s%6"U32_F"%6"U32_F"%6"U32_F"%6"U32_F"\n",name,
	 (u32_t)mem->avail,(u32_t)mem->used,(u32_t)mem->max,(u32_t)mem->err);
}

#if MEMP_STATS
void stats_display_memp(struct stats_mem *mem, int index)
{
  if (index < MEMP_MAX) {
    stats_display_mem(mem, mem->name);
  }
}
#endif /* MEMP_STATS */
#endif /* MEM_STATS || MEMP_STATS */

#if SYS_STATS
void stats_display_sys(struct stats_sys *sys)
{
	printf("sem  :%6"U32_F"%6"U32_F"%6"U32_F"\n", (u32_t)sys->sem.used,
	  (u32_t)sys->sem.max,(u32_t)sys->sem.err);

	printf("mutex:%6"U32_F"%6"U32_F"%6"U32_F"\n", (u32_t)sys->mutex.used,
	  (u32_t)sys->mutex.max,(u32_t)sys->mutex.err);

	printf("mbox :%6"U32_F"%6"U32_F"%6"U32_F"\n", (u32_t)sys->mbox.used,
	  (u32_t)sys->mbox.max,(u32_t)sys->mbox.err);
}
#endif /* SYS_STATS */

void stats_display(void)
{
  s16_t i;

	printf("\r\n\t\ttxmit\trecv\tfw\tdrop\tchkerr\tlenerr\tmemerr\
\trterr\tproterr\topterr\terr\tcachehit\n");

	LINK_STATS_DISPLAY();
	ETHARP_STATS_DISPLAY();
	IPFRAG_STATS_DISPLAY();
	IP6_FRAG_STATS_DISPLAY();
	IP_STATS_DISPLAY();
	ND6_STATS_DISPLAY();
	IP6_STATS_DISPLAY();
	MLD6_STATS_DISPLAY();
	ICMP_STATS_DISPLAY();
	ICMP6_STATS_DISPLAY();
	UDP_STATS_DISPLAY();
	TCP_STATS_DISPLAY();
	MEM_STATS_DISPLAY();
	printf("===================================================\n");
	printf("memp type:       avail\tused\tmax\terr\t\n");
	for (i = 0; i < MEMP_MAX; i++) {
		MEMP_STATS_DISPLAY(i);
	}
	printf("===================================================\n");
	printf("sys type: used\tmax\terr\n");
	SYS_STATS_DISPLAY();
}
#endif /* LWIP_STATS_DISPLAY */

#endif /* LWIP_STATS */

