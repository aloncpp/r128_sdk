
////////////////////////////////
//scr_hal.c
////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hal_timer.h>
#include <irqs.h>
#include "scr_hal.h"
#include "scr_hal_function.h"
#include <interrupt.h>
#include <console.h>

//#ifdef CONFIG_SCR_TEST

#define SCR_HAL_DBG 			printf
#define SCR_HAL_INFO  			printf
#define SMARTCARD_INFO  		printf

extern scr_struct scr;
//extern scr_fsm_record scr_fsm;

scr_test_stage stage = sts_idle;

#define SCR_SIM_DBG(x) 	pattern_mod_goto(0x2, x)//sys_sim_pio_debug(SYS_SIM_SCR_PID, x)
#define get_wvalue	readl

uint32_t smartcard_params_init(pscatr_struct pscatr)
{
	pscatr->TS = 0x3B;
	pscatr->TK_NUM = 0x00;

	pscatr->T = 0;		//T=0 Protocol
	pscatr->FMAX = 4;   //4MHz
	pscatr->F = 372;
	pscatr->D = 1;
	pscatr->I = 50;     //50mA
	pscatr->P = 5;      //5V
	pscatr->N = 2;

	return 0;
}

void smartcard_ta1_decode(pscatr_struct pscatr, uint8_t ta1)
{
	uint8_t temp = ta1;

	switch((temp >> 4) & 0xf)
	{
		case 0x0:
			pscatr->FMAX = 4;
			pscatr->F = 372;
			break;
		case 0x1:
			pscatr->FMAX = 5;
			pscatr->F = 372;
			break;
		case 0x2:
			pscatr->FMAX = 6;
			pscatr->F = 558;
			break;
		case 0x3:
			pscatr->FMAX = 8;
			pscatr->F = 744;
			break;
		case 0x4:
			pscatr->FMAX = 12;
			pscatr->F = 1116;
			break;
		case 0x5:
			pscatr->FMAX = 16;
			pscatr->F = 1488;
			break;
		case 0x6:
			pscatr->FMAX = 20;
			pscatr->F = 1860;
			break;
		case 0x9:
			pscatr->FMAX = 5;
			pscatr->F = 512;
			break;
		case 0xA:
			pscatr->FMAX = 7;
			pscatr->F = 768;
			break;
		case 0xB:
			pscatr->FMAX = 10;
			pscatr->F = 1024;
			break;
		case 0xC:
			pscatr->FMAX = 15;
			pscatr->F = 1536;
			break;
		case 0xD:
			pscatr->FMAX = 20;
			pscatr->F = 2048;
			break;
		default:  //0x7/0x8/0xE/0xF
			pscatr->FMAX = 4;
			pscatr->F = 372;
			SMARTCARD_INFO("Unsupport ta1 = 0x%x\n", ta1);
			break;
	}

	switch(temp&0xf)
	{
		case 0x1:
			pscatr->D = 1;
			break;
		case 0x2:
			pscatr->D = 2;
			break;
		case 0x3:
			pscatr->D = 4;
			break;
		case 0x4:
			pscatr->D = 8;
			break;
		case 0x5:
			pscatr->D = 16;
			break;
		case 0x6:
			pscatr->D = 32;
			break;
		case 0x8:
			pscatr->D = 12;
			break;
		case 0x9:
			pscatr->D = 20;
			break;
		default: //0x0/0x7/0xA/0xB/0xC/0xD/0xE/0xF
			pscatr->D = 1;
			SMARTCARD_INFO("Unsupport ta1 = 0x%x\n", ta1);
			break;
	}
}

void smartcard_tb1_decode(pscatr_struct pscatr, uint8_t tb1)
{
	uint8_t temp = tb1;

	switch((temp>>5)&0x3)
	{
		case 0:
			pscatr->I = 25;
			break;
		case 1:
			pscatr->I = 50;
			break;
		case 2:
			pscatr->I = 100;
			break;
		default:
			pscatr->I = 50;
	}

	if(((temp&0x1f)>4)&&((temp&0x1f)<26))
	{
		pscatr->P = (temp&0x1f); //5~25 in Volts
	}
	else if((temp&0x1f)==0)
	{
		pscatr->P = 0;  //NC
	}
	else
	{
		pscatr->P = 5;  //NC
	}
}

//ATR序列:TS，T0，TA1，TB1，TC1，TD1，TA2，TB2，TC2，TD2，... ,TAi,TBi,TCi,TDi,T1，T2，T3，... ,TK,TCK
uint32_t smartcard_atr_decode(pscatr_struct pscatr, uint8_t* pdata, ppps_struct pps, uint32_t with_ts)
{
	uint32_t index=0;
	uint8_t temp;
	uint32_t i;

	pps->ppss = 0xff;  //PPSS
	pps->pps0 = 0;

	if(with_ts)
	{
		pscatr->TS = pdata[0]; //TS
		index ++;
	}
	temp = pdata[index]; //T0
	index ++;
	pscatr->TK_NUM = temp & 0xf;   //获取历史字节的字节数

	if(temp & 0x10) //TA1
	{
		smartcard_ta1_decode(pscatr, pdata[index]);
		pps->pps0 |= 0x01<<4;        //指明pps1存在
		pps->pps1 = pdata[index];   //pps1=TA1，继续采用Fd和Dd
		index ++;
	}
	if(temp & 0x20) //TB1
	{
		smartcard_tb1_decode(pscatr, pdata[index]);
		index++;
	}
	if(temp & 0x40) //TC1
	{
		pscatr->N = pdata[index] & 0xff;  //获取额外保护时间
		index ++;
	}
	if(temp & 0x80) //TD1
	{
		temp = pdata[index];
		pscatr->T = temp & 0xf;
		pps->pps0 |= temp & 0xf;   //指明使用的协议,T=0或者T=1
		if(pscatr->N == 0xff)  //N = 255,it stands for: when T=0,guard time=2 etu;
		{                      //                       when T=1,guard time=1 etu.
			if(pscatr->T == 1) pscatr->N = 1;   //T=1,guard time = 1 etu
			else			   pscatr->N = 2;   //T=0,guard time = 2 etu
		}
		index ++;
	}
	else   //当IC卡不回复TD1时，默认使用T=0协议，后续传输协议缺省为T=0
	{
		if(pscatr->N == 0xff) pscatr->N = 2;   //when T=0, guard time=2 etu.
		goto rx_tk;
	}

	if(temp & 0x10)  //TA2
	{
		SMARTCARD_INFO("TA2 Exist!!\n");
		index ++;
	}
	if(temp & 0x20)  //TB2
	{
		SMARTCARD_INFO("TB2 Exist!!\n");
		index ++;
	}
	if(temp & 0x40)  //TC2
	{
		SMARTCARD_INFO("TC2 Exist!!\n");
		index ++;
	}
	if(temp & 0x80)  //TD2
	{
		SMARTCARD_INFO("TD2 Exist!!\n");
		temp = pdata[index];
		index ++;
	}
	else
	{
		goto rx_tk;
	}

	if(temp & 0x10)  //TA3
	{
		SMARTCARD_INFO("TA3 Exist!!\n");
		index ++;
	}
	if(temp & 0x20)  //TB3
	{
		SMARTCARD_INFO("TB3 Exist!!\n");
		index ++;
	}
	if(temp & 0x40)  //TC3
	{
		SMARTCARD_INFO("TC3 Exist!!\n");
		index ++;
	}
	if(temp & 0x80)  //TD3
	{
		SMARTCARD_INFO("TD3 Exist!!\n");
		temp = pdata[index];
		index ++;
	}
	else
	{
		goto rx_tk;
	}

	if(temp & 0x10)  //TA4
	{
		SMARTCARD_INFO("TA4 Exist!!\n");
		index ++;
	}
	if(temp & 0x20)  //TB4
	{
		SMARTCARD_INFO("TB4 Exist!!\n");
		index ++;
	}
	if(temp & 0x40)  //TC4
	{
		SMARTCARD_INFO("TC4 Exist!!\n");
		index ++;
	}
	if(temp & 0x80)  //TD4
	{
		SMARTCARD_INFO("TD4 Exist!!\n");
		temp = pdata[index];
		index ++;
	}
	else
	{
		goto rx_tk;
	}

rx_tk:   //获取历史字节
	for(i=0; i<(pscatr->TK_NUM); i++)
	{
		pscatr->TK[i] = pdata[index++];
	}

	pps->pck = pps->ppss;
	pps->pck ^= pps->pps0;
	if(pps->pps0&(0x1<<4))
	{
		pps->pck ^= pps->pps1;
	}
	if(pps->pps0&(0x1<<5))
	{
		pps->pck ^= pps->pps2;
	}
	if(pps->pps0&(0x1<<6))
	{
		pps->pck ^= pps->pps3;
	}

	return 0;
}


uint32_t scr_init(pscr_struct pscr)    //init SCR
{
	scr_global_interrupt_disable(pscr);

	scr_set_csr_config(pscr, pscr->csr_config);

	scr_flush_txfifo(pscr);
	scr_flush_rxfifo(pscr);

	scr_set_txfifo_threshold(pscr, pscr->txfifo_thh);
	scr_set_rxfifo_threshold(pscr, pscr->rxfifo_thh);

	scr_set_tx_repeat(pscr, pscr->tx_repeat);/*This is a 4-bit register that specifies the number of attempts to request character re-transmission after wrong parity was detected*/
	scr_set_rx_repeat(pscr, pscr->rx_repeat);

	scr_set_scclk_divisor(pscr, pscr->scclk_div);
	scr_set_baud_divisor(pscr, pscr->baud_div);
	scr_set_activation_time(pscr, pscr->act_time);
	scr_set_reset_time(pscr, pscr->rst_time);
	scr_set_atrlimit_time(pscr, pscr->atr_time);
	scr_set_guard_time(pscr, pscr->guard_time);
	scr_set_chlimit_time(pscr, pscr->chlimit_time);

	scr_set_debounce_time(pscr, pscr->debounce_time);



	scr_receive_enable(pscr);
	scr_transmit_enable(pscr);
	scr_auto_vpp_enable(pscr);

	pscr->irq_accsta = 0x00;
	pscr->irq_cursta = 0x00;
	pscr->irq_flag = 0;
	pscr->rxbuf.rptr = 0;
	pscr->rxbuf.wptr = 0;
	pscr->txbuf.rptr = 0;
	pscr->txbuf.wptr = 0;

	pscr->detected = 0;
	pscr->activated = 0;
	pscr->atr_resp = SCR_ATR_RESP_INVALID;

	pscr->chto_flag = 0;

	scr_clear_interrupt_status(pscr, 0xffffffff);
	scr_set_interrupt_disable(pscr, 0xffffffff);
	scr_set_interrupt_enable(pscr, pscr->inten_bm);

	scr_global_interrupt_enable(&scr);

	return 0;
}

void scr_fill_buffer(pscr_buffer pbuf, uint8_t data)   //fill data in the buffer of the pscr_struct
{
	pbuf->buffer[pbuf->wptr] = data;
	pbuf->wptr ++;
	if((pbuf->wptr) == SCR_BUFFER_SIZE_MASK+1)
		pbuf->wptr = 0;
}

uint8_t scr_dump_buffer(pscr_buffer pbuf)   //read data in the buffer of the pscr_struct
{
	uint8_t data;

	data = pbuf->buffer[pbuf->rptr&SCR_BUFFER_SIZE_MASK];
	pbuf->rptr ++;
	pbuf->rptr &= (SCR_BUFFER_SIZE_MASK<<1)|0x1;

	return data;
}

void scr_display_buffer_data(pscr_buffer pbuf)  //display the data in the buffer of the pscr_struct through uart
{
	uint32_t i = 0;

	if(!scr_buffer_is_empty(pbuf))
	{
		for(i=pbuf->rptr; (i&((SCR_BUFFER_SIZE_MASK<<1)|0x1)) != pbuf->wptr; i++)
		{
			printf("0x%x ", pbuf->buffer[i&SCR_BUFFER_SIZE_MASK]);

			/*edit by lrx*/
			(pbuf->rptr)++;
			if((pbuf->rptr)>=SCR_BUFFER_SIZE_MASK)
				pbuf->rptr = 0;
			/*edit by lrx*/
		}
		printf("\n");
	}
	else
	{
		printf("Buffer is Empty when Display!!\n");
	}
}


u32 scr_rx_fifo_read(u8 *buffer)
{
	u32 i = 0;

	while(scr.rxbuf.wptr != scr.rxbuf.rptr)
	{
		if(scr.rxbuf.wptr < scr.rxbuf.rptr)
		{
			for(i=0; i<(SCR_BUFFER_SIZE_MASK+1 - scr.rxbuf.rptr); i++)
			{
				*(buffer++) = *((uint8_t*)(scr.rxbuf.buffer+scr.rxbuf.rptr+i));
			}
			for(i=0; i<scr.rxbuf.wptr; i++)
			{
				*(buffer++) = *((uint8_t*)(scr.rxbuf.buffer+i));
			}
			scr.rxbuf.rptr = scr.rxbuf.wptr;
		}
		else
		{
			for(i=0; i<(scr.rxbuf.wptr - scr.rxbuf.rptr); i++)
			{
				*(buffer++) = *((uint8_t*)(scr.rxbuf.buffer+scr.rxbuf.rptr+i));
			}
			scr.rxbuf.rptr = scr.rxbuf.wptr;
		}
	}

	return 1;
}


void scr_handler_irq(pscr_struct pscr)   //record interrupt status and then clear them
{

	uint32_t temp = scr_get_interrupt_status(pscr);
	scr_clear_interrupt_status(pscr, temp);

	pscr->irq_accsta |= temp;
	pscr->irq_cursta = temp;

	if(pscr->irq_accsta & SCR_INTSTA_INS)   //ic卡插入中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_INS;
		if(get_wvalue(0x40045400)&0x80000000)		/* Smart Card Detected --- Input is active at least for a debounce time */
		{
			pscr->detected = 1;
			pscr->atr_resp = SCR_ATR_RESP_INVALID;
			stage = sts_wait_connect;
			SCR_HAL_INFO("SmartCard Inserted!!\n");
		}
	}

	if(pscr->irq_accsta & SCR_INTSTA_REM)  //ic卡移除中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_REM;
		if(!(get_wvalue(0x40045400)&0x80000000))
		{
			pscr->detected = 0;
			pscr->atr_resp = SCR_ATR_RESP_INVALID;
			stage = sts_start_deact;
			SCR_HAL_INFO("SmartCard Removed!!\n");
		}
	}

	if(pscr->irq_accsta & SCR_INTSTA_ACT)  //ic卡激活中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_ACT;
		if(get_wvalue(0x40045400)&0x80000000)
		{
			pscr->activated = 1;
			pscr->atr_resp = SCR_ATR_RESP_INVALID;
			SCR_HAL_INFO("SmartCard Activated!!\n");
		}
	}

	if(pscr->irq_accsta & SCR_INTSTA_DEACT)  //ic卡失活中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_DEACT;
		pscr->activated = 0;
		pscr->atr_resp = SCR_ATR_RESP_INVALID;
		SCR_HAL_INFO("SmartCard Deactivated!!\n");

	}

	if(pscr->irq_accsta & SCR_INTSTA_ATRFAIL)   //ATR失败中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_ATRFAIL;
		pscr->atr_resp = SCR_ATR_RESP_FAIL;
		printf("SmartCard ATR fail!!\n");
	}

	if(pscr->irq_accsta & SCR_INTSTA_ATRDONE)  //ATR完成中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_ATRDONE;
		pscr->atr_resp = SCR_ATR_RESP_OK;
		printf("SmartCard ATR done!!\n");
	}

	if(pscr->irq_accsta & SCR_INTSTA_RXFTH)
	{
		pscr->irq_accsta &= ~SCR_INTSTA_RXFTH;

		while(!scr_rxfifo_is_empty(pscr))
		{
			scr_fill_buffer(&pscr->rxbuf, scr_read_fifo(pscr));
		}
	}

	if(pscr->irq_accsta & SCR_INTSTA_RXDONE)
	{
		pscr->irq_accsta &= ~SCR_INTSTA_RXDONE;

		while(!scr_rxfifo_is_empty(pscr))
		{
			scr_fill_buffer(&pscr->rxbuf, scr_read_fifo(pscr));
		}
	}


	if(pscr->irq_accsta & SCR_INTSTA_TXFDONE)  //ic卡txfifo中所有bytes发送完成中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_TXFDONE;
	}

	if(pscr->irq_accsta & SCR_INTSTA_TXFEMPTY)  //ic卡txfifo空中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_TXFEMPTY;
	}

	if(pscr->irq_accsta & SCR_INTSTA_TXFTH)   //ic卡txfifo阈值触发中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_TXFTH;
	}

	if(pscr->irq_accsta & SCR_INTSTA_TXDONE)  //ic卡txfifo中1个字节发送完成中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_TXDONE;
	}

	if(pscr->irq_accsta & SCR_INTSTA_TXPERR)  //ic卡tx校验错误
	{
		pscr->irq_accsta &= ~SCR_INTSTA_TXPERR;
	}

	if(pscr->irq_accsta & SCR_INTSTA_RXFFULL)
	{
		pscr->irq_accsta &= ~SCR_INTSTA_RXFFULL;
	}

	if(pscr->irq_accsta & SCR_INTSTA_RXPERR)
	{
		pscr->irq_accsta &= ~SCR_INTSTA_RXPERR;
	}

	if(pscr->irq_accsta & SCR_INTSTA_CLOCK)  //scr时钟停,时钟开  中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_CLOCK;
	}

	if(pscr->irq_accsta & SCR_INTSTA_CHTO)   //字节时间超时中断
	{
		pscr->irq_accsta &= ~SCR_INTSTA_CHTO;
		pscr->chto_flag ++;
	}

}

void scr_fsm_record_start(pscr_struct pscr, pscr_fsm_record pfsm)
{
	pfsm->count = 1;
	pfsm->old = scr_get_fsm(pscr);
	pfsm->record[0] = pfsm->old;
}

void scr_fsm_record_run(pscr_struct pscr, pscr_fsm_record pfsm)
{
	uint32_t temp;

	if(pfsm->count >= SCR_FSM_MAX_RECORD) return;
	temp = scr_get_fsm(pscr);
	if(pfsm->old != temp)
	{
		pfsm->old = temp;
		pfsm->record[pfsm->count] = temp;
		pfsm->count ++;
	}
}

//void scr_fsm_decode(uint32_t val)
//{
//
//}

//uint32_t scr_test_interrupt_status(pscr_struct pscr)  //display the interrupt status through uart
//{
//	uint32_t temp = scr_get_interrupt_status(pscr);
//
//	scr_clear_interrupt_status(pscr, temp);
//	if(temp)
//	{
//		pscr->irq_cursta = temp;
//		pscr->irq_accsta |= temp;
//		SCR_HAL_DBG("IRQ Status = 0x%x\n", pscr->irq_cursta);
//	}
//
//	return temp;
//}

//#endif
