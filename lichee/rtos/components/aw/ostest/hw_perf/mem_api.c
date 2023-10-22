#include <stdio.h>
#include <sunxi_hal_common.h>

int sram_dbus_cpu_write_512k(uint32_t start_addr)
{
	int addr = 0, i = 0;
	do{
		addr = start_addr + (i<<11);
		writel(0x5a5a1234,addr + 0x0000);
		writel(0x5a5a1234,addr + 0x0004);
		writel(0x5a5a1234,addr + 0x0008);
		writel(0x5a5a1234,addr + 0x000c);
		writel(0x5a5a1234,addr + 0x0010);
		writel(0x5a5a1234,addr + 0x0014);
		writel(0x5a5a1234,addr + 0x0018);
		writel(0x5a5a1234,addr + 0x001c);
		writel(0x5a5a1234,addr + 0x0020);
		writel(0x5a5a1234,addr + 0x0024);
		writel(0x5a5a1234,addr + 0x0028);
		writel(0x5a5a1234,addr + 0x002c);
		writel(0x5a5a1234,addr + 0x0030);
		writel(0x5a5a1234,addr + 0x0034);
		writel(0x5a5a1234,addr + 0x0038);
		writel(0x5a5a1234,addr + 0x003c);
		writel(0x5a5a1234,addr + 0x0040);
		writel(0x5a5a1234,addr + 0x0044);
		writel(0x5a5a1234,addr + 0x0048);
		writel(0x5a5a1234,addr + 0x004c);
		writel(0x5a5a1234,addr + 0x0050);
		writel(0x5a5a1234,addr + 0x0054);
		writel(0x5a5a1234,addr + 0x0058);
		writel(0x5a5a1234,addr + 0x005c);
		writel(0x5a5a1234,addr + 0x0060);
		writel(0x5a5a1234,addr + 0x0064);
		writel(0x5a5a1234,addr + 0x0068);
		writel(0x5a5a1234,addr + 0x006c);
		writel(0x5a5a1234,addr + 0x0070);
		writel(0x5a5a1234,addr + 0x0074);
		writel(0x5a5a1234,addr + 0x0078);
		writel(0x5a5a1234,addr + 0x007c);
		writel(0x5a5a1234,addr + 0x0080);
		writel(0x5a5a1234,addr + 0x0084);
		writel(0x5a5a1234,addr + 0x0088);
		writel(0x5a5a1234,addr + 0x008c);
		writel(0x5a5a1234,addr + 0x0090);
		writel(0x5a5a1234,addr + 0x0094);
		writel(0x5a5a1234,addr + 0x0098);
		writel(0x5a5a1234,addr + 0x009c);
		writel(0x5a5a1234,addr + 0x00a0);
		writel(0x5a5a1234,addr + 0x00a4);
		writel(0x5a5a1234,addr + 0x00a8);
		writel(0x5a5a1234,addr + 0x00ac);
		writel(0x5a5a1234,addr + 0x00b0);
		writel(0x5a5a1234,addr + 0x00b4);
		writel(0x5a5a1234,addr + 0x00b8);
		writel(0x5a5a1234,addr + 0x00bc);
		writel(0x5a5a1234,addr + 0x00c0);
		writel(0x5a5a1234,addr + 0x00c4);
		writel(0x5a5a1234,addr + 0x00c8);
		writel(0x5a5a1234,addr + 0x00cc);
		writel(0x5a5a1234,addr + 0x00d0);
		writel(0x5a5a1234,addr + 0x00d4);
		writel(0x5a5a1234,addr + 0x00d8);
		writel(0x5a5a1234,addr + 0x00dc);
		writel(0x5a5a1234,addr + 0x00e0);
		writel(0x5a5a1234,addr + 0x00e4);
		writel(0x5a5a1234,addr + 0x00e8);
		writel(0x5a5a1234,addr + 0x00ec);
		writel(0x5a5a1234,addr + 0x00f0);
		writel(0x5a5a1234,addr + 0x00f4);
		writel(0x5a5a1234,addr + 0x00f8);
		writel(0x5a5a1234,addr + 0x00fc);

		writel(0x5a5a1234,addr + 0x0100);
		writel(0x5a5a1234,addr + 0x0104);
		writel(0x5a5a1234,addr + 0x0108);
		writel(0x5a5a1234,addr + 0x010c);
		writel(0x5a5a1234,addr + 0x0110);
		writel(0x5a5a1234,addr + 0x0114);
		writel(0x5a5a1234,addr + 0x0118);
		writel(0x5a5a1234,addr + 0x011c);
		writel(0x5a5a1234,addr + 0x0120);
		writel(0x5a5a1234,addr + 0x0124);
		writel(0x5a5a1234,addr + 0x0128);
		writel(0x5a5a1234,addr + 0x012c);
		writel(0x5a5a1234,addr + 0x0130);
		writel(0x5a5a1234,addr + 0x0134);
		writel(0x5a5a1234,addr + 0x0138);
		writel(0x5a5a1234,addr + 0x013c);
		writel(0x5a5a1234,addr + 0x0140);
		writel(0x5a5a1234,addr + 0x0144);
		writel(0x5a5a1234,addr + 0x0148);
		writel(0x5a5a1234,addr + 0x014c);
		writel(0x5a5a1234,addr + 0x0150);
		writel(0x5a5a1234,addr + 0x0154);
		writel(0x5a5a1234,addr + 0x0158);
		writel(0x5a5a1234,addr + 0x015c);
		writel(0x5a5a1234,addr + 0x0160);
		writel(0x5a5a1234,addr + 0x0164);
		writel(0x5a5a1234,addr + 0x0168);
		writel(0x5a5a1234,addr + 0x016c);
		writel(0x5a5a1234,addr + 0x0170);
		writel(0x5a5a1234,addr + 0x0174);
		writel(0x5a5a1234,addr + 0x0178);
		writel(0x5a5a1234,addr + 0x017c);
		writel(0x5a5a1234,addr + 0x0180);
		writel(0x5a5a1234,addr + 0x0184);
		writel(0x5a5a1234,addr + 0x0188);
		writel(0x5a5a1234,addr + 0x018c);
		writel(0x5a5a1234,addr + 0x0190);
		writel(0x5a5a1234,addr + 0x0194);
		writel(0x5a5a1234,addr + 0x0198);
		writel(0x5a5a1234,addr + 0x019c);
		writel(0x5a5a1234,addr + 0x01a0);
		writel(0x5a5a1234,addr + 0x01a4);
		writel(0x5a5a1234,addr + 0x01a8);
		writel(0x5a5a1234,addr + 0x01ac);
		writel(0x5a5a1234,addr + 0x01b0);
		writel(0x5a5a1234,addr + 0x01b4);
		writel(0x5a5a1234,addr + 0x01b8);
		writel(0x5a5a1234,addr + 0x01bc);
		writel(0x5a5a1234,addr + 0x01c0);
		writel(0x5a5a1234,addr + 0x01c4);
		writel(0x5a5a1234,addr + 0x01c8);
		writel(0x5a5a1234,addr + 0x01cc);
		writel(0x5a5a1234,addr + 0x01d0);
		writel(0x5a5a1234,addr + 0x01d4);
		writel(0x5a5a1234,addr + 0x01d8);
		writel(0x5a5a1234,addr + 0x01dc);
		writel(0x5a5a1234,addr + 0x01e0);
		writel(0x5a5a1234,addr + 0x01e4);
		writel(0x5a5a1234,addr + 0x01e8);
		writel(0x5a5a1234,addr + 0x01ec);
		writel(0x5a5a1234,addr + 0x01f0);
		writel(0x5a5a1234,addr + 0x01f4);
		writel(0x5a5a1234,addr + 0x01f8);
		writel(0x5a5a1234,addr + 0x01fc);

		writel(0x5a5a1234,addr + 0x0200);
		writel(0x5a5a1234,addr + 0x0204);
		writel(0x5a5a1234,addr + 0x0208);
		writel(0x5a5a1234,addr + 0x020c);
		writel(0x5a5a1234,addr + 0x0210);
		writel(0x5a5a1234,addr + 0x0214);
		writel(0x5a5a1234,addr + 0x0218);
		writel(0x5a5a1234,addr + 0x021c);
		writel(0x5a5a1234,addr + 0x0220);
		writel(0x5a5a1234,addr + 0x0224);
		writel(0x5a5a1234,addr + 0x0228);
		writel(0x5a5a1234,addr + 0x022c);
		writel(0x5a5a1234,addr + 0x0230);
		writel(0x5a5a1234,addr + 0x0234);
		writel(0x5a5a1234,addr + 0x0238);
		writel(0x5a5a1234,addr + 0x023c);
		writel(0x5a5a1234,addr + 0x0240);
		writel(0x5a5a1234,addr + 0x0244);
		writel(0x5a5a1234,addr + 0x0248);
		writel(0x5a5a1234,addr + 0x024c);
		writel(0x5a5a1234,addr + 0x0250);
		writel(0x5a5a1234,addr + 0x0254);
		writel(0x5a5a1234,addr + 0x0258);
		writel(0x5a5a1234,addr + 0x025c);
		writel(0x5a5a1234,addr + 0x0260);
		writel(0x5a5a1234,addr + 0x0264);
		writel(0x5a5a1234,addr + 0x0268);
		writel(0x5a5a1234,addr + 0x026c);
		writel(0x5a5a1234,addr + 0x0270);
		writel(0x5a5a1234,addr + 0x0274);
		writel(0x5a5a1234,addr + 0x0278);
		writel(0x5a5a1234,addr + 0x027c);
		writel(0x5a5a1234,addr + 0x0280);
		writel(0x5a5a1234,addr + 0x0284);
		writel(0x5a5a1234,addr + 0x0288);
		writel(0x5a5a1234,addr + 0x028c);
		writel(0x5a5a1234,addr + 0x0290);
		writel(0x5a5a1234,addr + 0x0294);
		writel(0x5a5a1234,addr + 0x0298);
		writel(0x5a5a1234,addr + 0x029c);
		writel(0x5a5a1234,addr + 0x02a0);
		writel(0x5a5a1234,addr + 0x02a4);
		writel(0x5a5a1234,addr + 0x02a8);
		writel(0x5a5a1234,addr + 0x02ac);
		writel(0x5a5a1234,addr + 0x02b0);
		writel(0x5a5a1234,addr + 0x02b4);
		writel(0x5a5a1234,addr + 0x02b8);
		writel(0x5a5a1234,addr + 0x02bc);
		writel(0x5a5a1234,addr + 0x02c0);
		writel(0x5a5a1234,addr + 0x02c4);
		writel(0x5a5a1234,addr + 0x02c8);
		writel(0x5a5a1234,addr + 0x02cc);
		writel(0x5a5a1234,addr + 0x02d0);
		writel(0x5a5a1234,addr + 0x02d4);
		writel(0x5a5a1234,addr + 0x02d8);
		writel(0x5a5a1234,addr + 0x02dc);
		writel(0x5a5a1234,addr + 0x02e0);
		writel(0x5a5a1234,addr + 0x02e4);
		writel(0x5a5a1234,addr + 0x02e8);
		writel(0x5a5a1234,addr + 0x02ec);
		writel(0x5a5a1234,addr + 0x02f0);
		writel(0x5a5a1234,addr + 0x02f4);
		writel(0x5a5a1234,addr + 0x02f8);
		writel(0x5a5a1234,addr + 0x02fc);

		writel(0x5a5a1234,addr + 0x0300);
		writel(0x5a5a1234,addr + 0x0304);
		writel(0x5a5a1234,addr + 0x0308);
		writel(0x5a5a1234,addr + 0x030c);
		writel(0x5a5a1234,addr + 0x0310);
		writel(0x5a5a1234,addr + 0x0314);
		writel(0x5a5a1234,addr + 0x0318);
		writel(0x5a5a1234,addr + 0x031c);
		writel(0x5a5a1234,addr + 0x0320);
		writel(0x5a5a1234,addr + 0x0324);
		writel(0x5a5a1234,addr + 0x0328);
		writel(0x5a5a1234,addr + 0x032c);
		writel(0x5a5a1234,addr + 0x0330);
		writel(0x5a5a1234,addr + 0x0334);
		writel(0x5a5a1234,addr + 0x0338);
		writel(0x5a5a1234,addr + 0x033c);
		writel(0x5a5a1234,addr + 0x0340);
		writel(0x5a5a1234,addr + 0x0344);
		writel(0x5a5a1234,addr + 0x0348);
		writel(0x5a5a1234,addr + 0x034c);
		writel(0x5a5a1234,addr + 0x0350);
		writel(0x5a5a1234,addr + 0x0354);
		writel(0x5a5a1234,addr + 0x0358);
		writel(0x5a5a1234,addr + 0x035c);
		writel(0x5a5a1234,addr + 0x0360);
		writel(0x5a5a1234,addr + 0x0364);
		writel(0x5a5a1234,addr + 0x0368);
		writel(0x5a5a1234,addr + 0x036c);
		writel(0x5a5a1234,addr + 0x0370);
		writel(0x5a5a1234,addr + 0x0374);
		writel(0x5a5a1234,addr + 0x0378);
		writel(0x5a5a1234,addr + 0x037c);
		writel(0x5a5a1234,addr + 0x0380);
		writel(0x5a5a1234,addr + 0x0384);
		writel(0x5a5a1234,addr + 0x0388);
		writel(0x5a5a1234,addr + 0x038c);
		writel(0x5a5a1234,addr + 0x0390);
		writel(0x5a5a1234,addr + 0x0394);
		writel(0x5a5a1234,addr + 0x0398);
		writel(0x5a5a1234,addr + 0x039c);
		writel(0x5a5a1234,addr + 0x03a0);
		writel(0x5a5a1234,addr + 0x03a4);
		writel(0x5a5a1234,addr + 0x03a8);
		writel(0x5a5a1234,addr + 0x03ac);
		writel(0x5a5a1234,addr + 0x03b0);
		writel(0x5a5a1234,addr + 0x03b4);
		writel(0x5a5a1234,addr + 0x03b8);
		writel(0x5a5a1234,addr + 0x03bc);
		writel(0x5a5a1234,addr + 0x03c0);
		writel(0x5a5a1234,addr + 0x03c4);
		writel(0x5a5a1234,addr + 0x03c8);
		writel(0x5a5a1234,addr + 0x03cc);
		writel(0x5a5a1234,addr + 0x03d0);
		writel(0x5a5a1234,addr + 0x03d4);
		writel(0x5a5a1234,addr + 0x03d8);
		writel(0x5a5a1234,addr + 0x03dc);
		writel(0x5a5a1234,addr + 0x03e0);
		writel(0x5a5a1234,addr + 0x03e4);
		writel(0x5a5a1234,addr + 0x03e8);
		writel(0x5a5a1234,addr + 0x03ec);
		writel(0x5a5a1234,addr + 0x03f0);
		writel(0x5a5a1234,addr + 0x03f4);
		writel(0x5a5a1234,addr + 0x03f8);
		writel(0x5a5a1234,addr + 0x03fc);
		i++;

	}while(i<512);
	return 0;

}
#ifdef CONFIG_ARCH_ARM
int psram_assembly_write(uint32_t start_addr, uint32_t len)
{
	__asm__ __volatile__ (          \
		"mov  r1, %0  \n\r"     \
		"mov  r2, %1  \n\r"     \
		"mov  r3, %0  \n\r"     \
		"add r4, %1, %0 \n\r"\
		"1: \n\r"		\
		"strd r2, r3, [r1, #0] \n\r"\
		"strd r2, r3, [r1, #8] \n\r"\
		"strd r2, r3, [r1, #16] \n\r" \ 
		"strd r2, r3, [r1, #24] \n\r" \ 
		"strd r2, r3, [r1, #32] \n\r" \ 
		"strd r2, r3, [r1, #40] \n\r" \ 
		"strd r2, r3, [r1, #48] \n\r" \ 
		"strd r2, r3, [r1, #56] \n\r" \ 
		"strd r2, r3, [r1, #64] \n\r" \ 
		"strd r2, r3, [r1, #72] \n\r" \ 
		"strd r2, r3, [r1, #80] \n\r" \ 
		"strd r2, r3, [r1, #88] \n\r" \ 
		"strd r2, r3, [r1, #96] \n\r" \ 
		"strd r2, r3, [r1, #104] \n\r" \ 
		"strd r2, r3, [r1, #112] \n\r" \ 
		"strd r2, r3, [r1, #120] \n\r" \ 
		"strd r2, r3, [r1, #128] \n\r" \ 
		"strd r2, r3, [r1, #136] \n\r" \ 
		"strd r2, r3, [r1, #144] \n\r" \ 
		"strd r2, r3, [r1, #152] \n\r" \ 
		"strd r2, r3, [r1, #160] \n\r" \ 
		"strd r2, r3, [r1, #168] \n\r" \ 
		"strd r2, r3, [r1, #176] \n\r" \ 
		"strd r2, r3, [r1, #184] \n\r" \ 
		"strd r2, r3, [r1, #192] \n\r" \ 
		"strd r2, r3, [r1, #200] \n\r" \ 
		"strd r2, r3, [r1, #208] \n\r" \ 
		"strd r2, r3, [r1, #216] \n\r" \ 
		"strd r2, r3, [r1, #224] \n\r" \ 
		"strd r2, r3, [r1, #232] \n\r" \ 
		"strd r2, r3, [r1, #240] \n\r" \ 
		"strd r2, r3, [r1, #248] \n\r" \ 
		"strd r2, r3, [r1, #256] \n\r" \ 
		"strd r2, r3, [r1, #264] \n\r" \ 
		"strd r2, r3, [r1, #272] \n\r" \ 
		"strd r2, r3, [r1, #280] \n\r" \ 
		"strd r2, r3, [r1, #288] \n\r" \ 
		"strd r2, r3, [r1, #296] \n\r" \ 
		"strd r2, r3, [r1, #304] \n\r" \ 
		"strd r2, r3, [r1, #312] \n\r" \ 
		"strd r2, r3, [r1, #320] \n\r" \ 
		"strd r2, r3, [r1, #328] \n\r" \ 
		"strd r2, r3, [r1, #336] \n\r" \ 
		"strd r2, r3, [r1, #344] \n\r" \ 
		"strd r2, r3, [r1, #352] \n\r" \ 
		"strd r2, r3, [r1, #360] \n\r" \ 
		"strd r2, r3, [r1, #368] \n\r" \ 
		"strd r2, r3, [r1, #376] \n\r" \ 
		"strd r2, r3, [r1, #384] \n\r" \ 
		"strd r2, r3, [r1, #392] \n\r" \ 
		"strd r2, r3, [r1, #400] \n\r" \ 
		"strd r2, r3, [r1, #408] \n\r" \ 
		"strd r2, r3, [r1, #416] \n\r" \ 
		"strd r2, r3, [r1, #424] \n\r" \ 
		"strd r2, r3, [r1, #432] \n\r" \ 
		"strd r2, r3, [r1, #440] \n\r" \ 
		"strd r2, r3, [r1, #448] \n\r" \ 
		"strd r2, r3, [r1, #456] \n\r" \ 
		"strd r2, r3, [r1, #464] \n\r" \ 
		"strd r2, r3, [r1, #472] \n\r" \ 
		"strd r2, r3, [r1, #480] \n\r" \ 
		"strd r2, r3, [r1, #488] \n\r" \ 
		"strd r2, r3, [r1, #496] \n\r" \ 
		"strd r2, r3, [r1, #504] \n\r" \ 
		"strd r2, r3, [r1, #512] \n\r" \ 
		"strd r2, r3, [r1, #520] \n\r" \ 
		"strd r2, r3, [r1, #528] \n\r" \ 
		"strd r2, r3, [r1, #536] \n\r" \ 
		"strd r2, r3, [r1, #544] \n\r" \ 
		"strd r2, r3, [r1, #552] \n\r" \ 
		"strd r2, r3, [r1, #560] \n\r" \ 
		"strd r2, r3, [r1, #568] \n\r" \ 
		"strd r2, r3, [r1, #576] \n\r" \ 
		"strd r2, r3, [r1, #584] \n\r" \ 
		"strd r2, r3, [r1, #592] \n\r" \ 
		"strd r2, r3, [r1, #600] \n\r" \ 
		"strd r2, r3, [r1, #608] \n\r" \ 
		"strd r2, r3, [r1, #616] \n\r" \ 
		"strd r2, r3, [r1, #624] \n\r" \ 
		"strd r2, r3, [r1, #632] \n\r" \ 
		"strd r2, r3, [r1, #640] \n\r" \ 
		"strd r2, r3, [r1, #648] \n\r" \ 
		"strd r2, r3, [r1, #656] \n\r" \ 
		"strd r2, r3, [r1, #664] \n\r" \ 
		"strd r2, r3, [r1, #672] \n\r" \ 
		"strd r2, r3, [r1, #680] \n\r" \ 
		"strd r2, r3, [r1, #688] \n\r" \ 
		"strd r2, r3, [r1, #696] \n\r" \ 
		"strd r2, r3, [r1, #704] \n\r" \ 
		"strd r2, r3, [r1, #712] \n\r" \ 
		"strd r2, r3, [r1, #720] \n\r" \ 
		"strd r2, r3, [r1, #728] \n\r" \ 
		"strd r2, r3, [r1, #736] \n\r" \ 
		"strd r2, r3, [r1, #744] \n\r" \ 
		"strd r2, r3, [r1, #752] \n\r" \ 
		"strd r2, r3, [r1, #760] \n\r" \ 
		"strd r2, r3, [r1, #768] \n\r" \ 
		"strd r2, r3, [r1, #776] \n\r" \ 
		"strd r2, r3, [r1, #784] \n\r" \ 
		"strd r2, r3, [r1, #792] \n\r" \ 
		"strd r2, r3, [r1, #800] \n\r" \ 
		"strd r2, r3, [r1, #808] \n\r" \ 
		"strd r2, r3, [r1, #816] \n\r" \ 
		"strd r2, r3, [r1, #824] \n\r" \ 
		"strd r2, r3, [r1, #832] \n\r" \ 
		"strd r2, r3, [r1, #840] \n\r" \ 
		"strd r2, r3, [r1, #848] \n\r" \ 
		"strd r2, r3, [r1, #856] \n\r" \ 
		"strd r2, r3, [r1, #864] \n\r" \ 
		"strd r2, r3, [r1, #872] \n\r" \ 
		"strd r2, r3, [r1, #880] \n\r" \ 
		"strd r2, r3, [r1, #888] \n\r" \ 
		"strd r2, r3, [r1, #896] \n\r" \ 
		"strd r2, r3, [r1, #904] \n\r" \ 
		"strd r2, r3, [r1, #912] \n\r" \ 
		"strd r2, r3, [r1, #920] \n\r" \ 
		"strd r2, r3, [r1, #928] \n\r" \ 
		"strd r2, r3, [r1, #936] \n\r" \ 
		"strd r2, r3, [r1, #944] \n\r" \ 
		"strd r2, r3, [r1, #952] \n\r" \ 
		"strd r2, r3, [r1, #960] \n\r" \ 
		"strd r2, r3, [r1, #968] \n\r" \ 
		"strd r2, r3, [r1, #976] \n\r" \ 
		"strd r2, r3, [r1, #984] \n\r" \ 
		"strd r2, r3, [r1, #992] \n\r" \ 
		"strd r2, r3, [r1, #1000] \n\r" \ 
		"strd r2, r3, [r1, #1008] \n\r" \ 
		"strd r2, r3, [r1, #1016] \n\r" \
		"add r1, r1, #1024\n\r"\
		"cmp r1, r4\n\r"\
		"bne 1b \n\r"\
		:: "r"(start_addr), "r"(len): \
		"memory", "r4", "r3", "r2", "r1", "cc");
	return 0;
}

int psram_assembly_read(uint32_t start_addr, uint32_t len)
{
	__asm__ __volatile__ (          \
		"mov  r1, %0  \n\r"     \
		"mov  r2, %1  \n\r"     \
		"add  r4, %0, %1 \n\r"  \
		"1: \n\r"               \
		"ldrd  r2, r3, [r1, #0]\n\r"\
		"ldrd  r2, r3, [r1, #8]\n\r"\
		"ldrd r2, r3, [r1, #16] \n\r" \ 
		"ldrd r2, r3, [r1, #24] \n\r" \ 
		"ldrd r2, r3, [r1, #32] \n\r" \ 
		"ldrd r2, r3, [r1, #40] \n\r" \ 
		"ldrd r2, r3, [r1, #48] \n\r" \ 
		"ldrd r2, r3, [r1, #56] \n\r" \ 
		"ldrd r2, r3, [r1, #64] \n\r" \ 
		"ldrd r2, r3, [r1, #72] \n\r" \ 
		"ldrd r2, r3, [r1, #80] \n\r" \ 
		"ldrd r2, r3, [r1, #88] \n\r" \ 
		"ldrd r2, r3, [r1, #96] \n\r" \ 
		"ldrd r2, r3, [r1, #104] \n\r" \ 
		"ldrd r2, r3, [r1, #112] \n\r" \ 
		"ldrd r2, r3, [r1, #120] \n\r" \ 
		"ldrd r2, r3, [r1, #128] \n\r" \ 
		"ldrd r2, r3, [r1, #136] \n\r" \ 
		"ldrd r2, r3, [r1, #144] \n\r" \ 
		"ldrd r2, r3, [r1, #152] \n\r" \ 
		"ldrd r2, r3, [r1, #160] \n\r" \ 
		"ldrd r2, r3, [r1, #168] \n\r" \ 
		"ldrd r2, r3, [r1, #176] \n\r" \ 
		"ldrd r2, r3, [r1, #184] \n\r" \ 
		"ldrd r2, r3, [r1, #192] \n\r" \ 
		"ldrd r2, r3, [r1, #200] \n\r" \ 
		"ldrd r2, r3, [r1, #208] \n\r" \ 
		"ldrd r2, r3, [r1, #216] \n\r" \ 
		"ldrd r2, r3, [r1, #224] \n\r" \ 
		"ldrd r2, r3, [r1, #232] \n\r" \ 
		"ldrd r2, r3, [r1, #240] \n\r" \ 
		"ldrd r2, r3, [r1, #248] \n\r" \ 
		"ldrd r2, r3, [r1, #256] \n\r" \ 
		"ldrd r2, r3, [r1, #264] \n\r" \ 
		"ldrd r2, r3, [r1, #272] \n\r" \ 
		"ldrd r2, r3, [r1, #280] \n\r" \ 
		"ldrd r2, r3, [r1, #288] \n\r" \ 
		"ldrd r2, r3, [r1, #296] \n\r" \ 
		"ldrd r2, r3, [r1, #304] \n\r" \ 
		"ldrd r2, r3, [r1, #312] \n\r" \ 
		"ldrd r2, r3, [r1, #320] \n\r" \ 
		"ldrd r2, r3, [r1, #328] \n\r" \ 
		"ldrd r2, r3, [r1, #336] \n\r" \ 
		"ldrd r2, r3, [r1, #344] \n\r" \ 
		"ldrd r2, r3, [r1, #352] \n\r" \ 
		"ldrd r2, r3, [r1, #360] \n\r" \ 
		"ldrd r2, r3, [r1, #368] \n\r" \ 
		"ldrd r2, r3, [r1, #376] \n\r" \ 
		"ldrd r2, r3, [r1, #384] \n\r" \ 
		"ldrd r2, r3, [r1, #392] \n\r" \ 
		"ldrd r2, r3, [r1, #400] \n\r" \ 
		"ldrd r2, r3, [r1, #408] \n\r" \ 
		"ldrd r2, r3, [r1, #416] \n\r" \ 
		"ldrd r2, r3, [r1, #424] \n\r" \ 
		"ldrd r2, r3, [r1, #432] \n\r" \ 
		"ldrd r2, r3, [r1, #440] \n\r" \ 
		"ldrd r2, r3, [r1, #448] \n\r" \ 
		"ldrd r2, r3, [r1, #456] \n\r" \ 
		"ldrd r2, r3, [r1, #464] \n\r" \ 
		"ldrd r2, r3, [r1, #472] \n\r" \ 
		"ldrd r2, r3, [r1, #480] \n\r" \ 
		"ldrd r2, r3, [r1, #488] \n\r" \ 
		"ldrd r2, r3, [r1, #496] \n\r" \ 
		"ldrd r2, r3, [r1, #504] \n\r" \ 
		"ldrd r2, r3, [r1, #512] \n\r" \ 
		"ldrd r2, r3, [r1, #520] \n\r" \ 
		"ldrd r2, r3, [r1, #528] \n\r" \ 
		"ldrd r2, r3, [r1, #536] \n\r" \ 
		"ldrd r2, r3, [r1, #544] \n\r" \ 
		"ldrd r2, r3, [r1, #552] \n\r" \ 
		"ldrd r2, r3, [r1, #560] \n\r" \ 
		"ldrd r2, r3, [r1, #568] \n\r" \ 
		"ldrd r2, r3, [r1, #576] \n\r" \ 
		"ldrd r2, r3, [r1, #584] \n\r" \ 
		"ldrd r2, r3, [r1, #592] \n\r" \ 
		"ldrd r2, r3, [r1, #600] \n\r" \ 
		"ldrd r2, r3, [r1, #608] \n\r" \ 
		"ldrd r2, r3, [r1, #616] \n\r" \ 
		"ldrd r2, r3, [r1, #624] \n\r" \ 
		"ldrd r2, r3, [r1, #632] \n\r" \ 
		"ldrd r2, r3, [r1, #640] \n\r" \ 
		"ldrd r2, r3, [r1, #648] \n\r" \ 
		"ldrd r2, r3, [r1, #656] \n\r" \ 
		"ldrd r2, r3, [r1, #664] \n\r" \ 
		"ldrd r2, r3, [r1, #672] \n\r" \ 
		"ldrd r2, r3, [r1, #680] \n\r" \ 
		"ldrd r2, r3, [r1, #688] \n\r" \ 
		"ldrd r2, r3, [r1, #696] \n\r" \ 
		"ldrd r2, r3, [r1, #704] \n\r" \ 
		"ldrd r2, r3, [r1, #712] \n\r" \ 
		"ldrd r2, r3, [r1, #720] \n\r" \ 
		"ldrd r2, r3, [r1, #728] \n\r" \ 
		"ldrd r2, r3, [r1, #736] \n\r" \ 
		"ldrd r2, r3, [r1, #744] \n\r" \ 
		"ldrd r2, r3, [r1, #752] \n\r" \ 
		"ldrd r2, r3, [r1, #760] \n\r" \ 
		"ldrd r2, r3, [r1, #768] \n\r" \ 
		"ldrd r2, r3, [r1, #776] \n\r" \ 
		"ldrd r2, r3, [r1, #784] \n\r" \ 
		"ldrd r2, r3, [r1, #792] \n\r" \ 
		"ldrd r2, r3, [r1, #800] \n\r" \ 
		"ldrd r2, r3, [r1, #808] \n\r" \ 
		"ldrd r2, r3, [r1, #816] \n\r" \ 
		"ldrd r2, r3, [r1, #824] \n\r" \ 
		"ldrd r2, r3, [r1, #832] \n\r" \ 
		"ldrd r2, r3, [r1, #840] \n\r" \ 
		"ldrd r2, r3, [r1, #848] \n\r" \ 
		"ldrd r2, r3, [r1, #856] \n\r" \ 
		"ldrd r2, r3, [r1, #864] \n\r" \ 
		"ldrd r2, r3, [r1, #872] \n\r" \ 
		"ldrd r2, r3, [r1, #880] \n\r" \ 
		"ldrd r2, r3, [r1, #888] \n\r" \ 
		"ldrd r2, r3, [r1, #896] \n\r" \ 
		"ldrd r2, r3, [r1, #904] \n\r" \ 
		"ldrd r2, r3, [r1, #912] \n\r" \ 
		"ldrd r2, r3, [r1, #920] \n\r" \ 
		"ldrd r2, r3, [r1, #928] \n\r" \ 
		"ldrd r2, r3, [r1, #936] \n\r" \ 
		"ldrd r2, r3, [r1, #944] \n\r" \ 
		"ldrd r2, r3, [r1, #952] \n\r" \ 
		"ldrd r2, r3, [r1, #960] \n\r" \ 
		"ldrd r2, r3, [r1, #968] \n\r" \ 
		"ldrd r2, r3, [r1, #976] \n\r" \ 
		"ldrd r2, r3, [r1, #984] \n\r" \ 
		"ldrd r2, r3, [r1, #992] \n\r" \ 
		"ldrd r2, r3, [r1, #1000] \n\r" \ 
		"ldrd r2, r3, [r1, #1008] \n\r" \ 
		"ldrd r2, r3, [r1, #1016] \n\r" \ 
		"add r1, r1, #1024\n\r"\ 
		"cmp r1, r4\n\r"\
		"bne  1b \n\r"          \
		:: "r"(start_addr), "r"(len): \
		"memory", "r4", "r3", "r2", "r1", "cc");
	return 0;
}
#else
int psram_assembly_write(uint32_t start_addr, uint32_t len)
{
	 __asm__ __volatile__ (          \
		"li   a3, 0 \n\r"       \
		"1: \n\r"               \
		"add  a4, %0, a3   \n\r"\
		"sd   a4, 0(a4)    \n\r"\
		"addi a3, a3, 8   \n\r" \
		"blt  a3, %1, 1b \n\r"  \
		: : "r"(start_addr), "r"(len):\
		"memory", "a4", "a3", "cc");
}

int psram_assembly_read(uint32_t start_addr, uint32_t len)
{
	__asm__ __volatile__ (          \
		"li   a3, 0 \n\r"       \
		"1: \n\r"               \
		"add  a4, %0, a3   \n\r"\
		"ld   a4, 0(a4)    \n\r"\
		"addi a3, a3, 8   \n\r" \
		"blt  a3, %1, 1b \n\r"  \
		: : "r"(start_addr), "r"(len):\
		"memory", "a4", "a3", "cc");
}
#endif
