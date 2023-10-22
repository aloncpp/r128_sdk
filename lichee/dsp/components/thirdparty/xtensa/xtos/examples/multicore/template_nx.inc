
// Copyright (c) 2018 Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
//
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.

// XTSC template include file for setting up the XTOS multi-core example.
// For use with NX cores.

#ifndef PROG_NAME
#define PROG_NAME mc_demo.exe
#endif

#ifndef XTSYSTEM
#define XTSYSTEM
#endif

#ifndef XTCORE
#define XTCORE
#endif

#ifndef XTPARAMS
#define XTPARAMS
#endif

// XTSC run parameters
-logging=xtsc_logger.txt
-no_default_memories

// Describe first core properties and parameters
-xtensa_system=$(XTSYSTEM)
-xtensa_core=$(XTCORE)
-xtensa_params=$(XTPARAMS)
-core_program=$(PROG_NAME)
-core_args=core0
-set_core_parm=ProcessorID=0
-set_core_parm=SimLogDisassembly=true
// Uncomment to generate ISS trace for core
// -set_core_parm=SimClients=trace --level 6 core0.trace
-create_core=core0

// Describe second core properties and parameters
-xtensa_system=$(XTSYSTEM)
-xtensa_core=$(XTCORE)
-xtensa_params=$(XTPARAMS)
-core_program=$(PROG_NAME)
-core_args=core1
-set_core_parm=ProcessorID=1
-set_core_parm=SimLogDisassembly=true
// Uncomment to generate ISS trace for core
// -set_core_parm=SimClients=trace --level 6 core1.trace
-create_core=core1

// Describe third core properties and parameters
-xtensa_system=$(XTSYSTEM)
-xtensa_core=$(XTCORE)
-xtensa_params=$(XTPARAMS)
-core_program=$(PROG_NAME)
-core_args=core2
-set_core_parm=ProcessorID=2
-set_core_parm=SimLogDisassembly=true
// Uncomment to generate ISS trace for core
// -set_core_parm=SimClients=trace --level 6 core2.trace
-create_core=core2

// Describe fourth core properties and parameters
-xtensa_system=$(XTSYSTEM)
-xtensa_core=$(XTCORE)
-xtensa_params=$(XTPARAMS)
-core_program=$(PROG_NAME)
-core_args=core3
-set_core_parm=ProcessorID=3
-set_core_parm=SimLogDisassembly=true
// Uncomment to generate ISS trace for core
// -set_core_parm=SimClients=trace --level 6 core3.trace
-create_core=core3

// Configure and create routers (connects core0 to mmio)
-set_router_parm=routing_table=routing.tab
-set_router_parm=num_slaves=2
-set_router_parm=default_port_num=0
-create_router=datamaster_rd_rte

-set_router_parm=routing_table=routing.tab
-set_router_parm=num_slaves=2
-set_router_parm=default_port_num=0
-create_router=datamaster_wr_rte

-set_router_parm=routing_table=routing.tab
-set_router_parm=num_slaves=2
-set_router_parm=default_port_num=0
-create_router=instmaster_rd_rte

// Configure and create mmio (allows core0 to control slave BReset)
-set_mmio_parm=definition_file=mmio.txt
-set_mmio_parm=byte_width=4
-set_mmio_parm=swizzle_bytes=false
-create_mmio=mmio

// configure and create arbiter for mmio
-set_arbiter_parm=num_masters=3
-create_arbiter=mmio_arb

// Configure and create arbiters for (shared) system memory
-set_arbiter_parm=num_masters=4
-create_arbiter=datamaster_rd_arb

-set_arbiter_parm=num_masters=4
-create_arbiter=datamaster_wr_arb

-set_arbiter_parm=num_masters=4
-create_arbiter=instmaster_rd_arb

// Configure and create system memory
-create_memory=sysmem

// Connect core 0 to routers
-connect=core0,datamaster_rd,slave_port,datamaster_rd_rte
-connect=core0,datamaster_wr,slave_port,datamaster_wr_rte
-connect=core0,instmaster_rd,slave_port,instmaster_rd_rte

// Connect routers to system memory arbiters
-connect=datamaster_rd_rte,master_port[0],slave_port[0],datamaster_rd_arb
-connect=datamaster_wr_rte,master_port[0],slave_port[0],datamaster_wr_arb
-connect=instmaster_rd_rte,master_port[0],slave_port[0],instmaster_rd_arb

// Connect routers to mmio arbiter
-connect=datamaster_rd_rte,master_port[1],slave_port[0],mmio_arb
-connect=datamaster_wr_rte,master_port[1],slave_port[1],mmio_arb
-connect=instmaster_rd_rte,master_port[1],slave_port[2],mmio_arb

// Connect mmio arbiter to mmio
-connect=mmio_arb,master_port,slave_port,mmio

// Connect mmio to core1-3 reset
-connect=mmio,SlaveReset1,BReset,core1
-connect=mmio,SlaveReset2,BReset,core2
-connect=mmio,SlaveReset3,BReset,core3

// Connect core1-3 to arbiters
-connect=core1,datamaster_rd,slave_port[1],datamaster_rd_arb
-connect=core1,datamaster_wr,slave_port[1],datamaster_wr_arb
-connect=core1,instmaster_rd,slave_port[1],instmaster_rd_arb

-connect=core2,datamaster_rd,slave_port[2],datamaster_rd_arb
-connect=core2,datamaster_wr,slave_port[2],datamaster_wr_arb
-connect=core2,instmaster_rd,slave_port[2],instmaster_rd_arb

-connect=core3,datamaster_rd,slave_port[3],datamaster_rd_arb
-connect=core3,datamaster_wr,slave_port[3],datamaster_wr_arb
-connect=core3,instmaster_rd,slave_port[3],instmaster_rd_arb

// Connect arbiters to system memory
-connect=datamaster_rd_arb,master_port,slave_port[0],sysmem
-connect=datamaster_wr_arb,master_port,slave_port[1],sysmem
-connect=instmaster_rd_arb,master_port,slave_port[2],sysmem

