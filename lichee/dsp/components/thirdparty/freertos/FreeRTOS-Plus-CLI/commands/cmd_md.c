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

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <aw_io.h>

/* FreeRTOS+CLI includes. */
#include "../FreeRTOS_CLI.h"

#define MAX_DUMP_LEN		0xff
#define VALID_ADDR_START	0x00000000
#define VALID_ADDR_END		0xffffffff

static bool _addr_is_valid(unsigned long addr)
{
	/* FIXME: valid address may be redefined? */
	if (addr >= VALID_ADDR_START && addr <= VALID_ADDR_END)
		return true;
	else
		return false;
}

static BaseType_t cmd_md_handler(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
	const char *paddr, *plen;
	char *pbuf = pcWriteBuffer;
	unsigned long start_addr, end_addr, len;
	BaseType_t xParameterStringLength;
	uint32_t i;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	(void)pcCommandString;
	(void)xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	memset(pcWriteBuffer, 0x00, xWriteBufferLen);

	/* Obtain the parameter string. */
	paddr = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
	plen = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

	/* parse parameters */
	len = strtoul(plen, NULL, 10);
	start_addr = strtoul(paddr, NULL, 16);

	if (!_addr_is_valid(start_addr)) {
		sprintf(pbuf, "Address out of bound!\r\n");
		return pdFALSE;
	}

	if (len > MAX_DUMP_LEN)
		len = MAX_DUMP_LEN;

	start_addr -= start_addr % 4; /* round down for 4 byte align */
	end_addr = start_addr + len;

	for (i = 0; start_addr <= end_addr; i++) {
		if (i % 4 == 0) { /* new line */
			sprintf(pbuf, "\r\n%08lx: ", start_addr);
			pbuf += 12;
		}

		sprintf(pbuf, "%08lx ", readl(start_addr));
		pbuf += 9;

		start_addr += 4;
	}

	return pdFALSE;
}

/*
 * Structure that defines the command.
 */
static const CLI_Command_Definition_t cmd_md = {
	"md",
	"md <start_addr> <len> : Dump memory from <start_addr> to <start_addr + len - 1>",
	cmd_md_handler,
	2
};

void cmd_md_register(void)
{
	FreeRTOS_CLIRegisterCommand(&cmd_md);
}
