
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
// For use with LX cores.

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

// Configure and create router (connects core0 to mmio)
-set_router_parm=routing_table=routing.tab
-set_router_parm=num_slaves=2
-set_router_parm=default_port_num=0
-create_router=router

// Configure and create mmio (allows core0 to control slave BReset)
-set_mmio_parm=definition_file=mmio.txt
-set_mmio_parm=byte_width=4
-set_mmio_parm=swizzle_bytes=false
-create_mmio=mmio

// Configure and create arbiter for (shared) system memory
-set_arbiter_parm=num_masters=4
-create_arbiter=arbiter

// Configure and create system memory
-memory_port=pif
-create_memory=sysmem

// Connect core0 to router
-connect=core0,pif,slave_port,router
// Connect router to system memory arbiter
-connect=router,master_port[0],slave_port[0],arbiter
// Connect router to mmio
-connect=router,master_port[1],slave_port,mmio

// Connect mmio to core1-3 reset
-connect=mmio,SlaveReset1,BReset,core1
-connect=mmio,SlaveReset2,BReset,core2
-connect=mmio,SlaveReset3,BReset,core3

// Connect core1-3 to arbiter
-connect=core1,pif,slave_port[1],arbiter
-connect=core2,pif,slave_port[2],arbiter
-connect=core3,pif,slave_port[3],arbiter

// Connect arbiter to system memory
-connect=arbiter,master_port,slave_port,sysmem

