
XTOS single-image multicore example
-----------------------------------

This example shows how to build and run a simple XTOS-based executable
that runs on multiple cores. Some core-specific customization may be
required for it to run, so please read this carefully before you try
to run the example. The basic requirements for single-image multicore
support are described in the Xtensa System Software Reference Manual.
Please check that your core configuration meets these requirements.

The example can be built and run using the "sim-mc" LSP provided with
the Xtensa software tools. However, shared access to the C library's
global data and locks imposes some requirements on the system. These
are the possible scenarios:

  1) Your system has no data caches. In this case cache coherence is
     not an issue and the sim-mc LSP can be used.

  2) Your system has data caches and hardware cache coherency. Again
     the sim-mc LSP can be used.

  3) Your system has data caches but no hardware coherency. However,
     you do not intend to use any of the C library functions that
     require data sharing/locking (file I/O, malloc/free etc.).
     In this case the C library global data and locks are irrelevant
     and the sim-mc LSP can be used.

  4) Your system has data caches but no hardware coherency, and you
     need to use C library functions that require data sharing/locking.
     In this case you need to create a custom LSP to provide a region
     of uncached shared memory. How to create such an LSP is described
     below. The MPU option is also required to set up the memory region
     as uncached.

This example can be run on the "sample_controller" core provided with 
Xtensa Xplorer. This core does not have a data cache so cache coherency
is not an issue. How to deal with data cache presence is described later.


What the example does
---------------------

The example shows core 0 starting up, initializing the system and then
starting cores 1-3. When all 4 cores are starting, they synchronize in
the main() function using an XTOS barrier construct. After this some
information is printed showing which core the code is running on, the
stack address, etc. A software interrupt handler is installed on each
core and the interrupt triggered, to show that interrupt dispatch and
handling happens independently on each core.

Since there is only one global stdout handle, all prints from every core
go to this output handle. The normal result is that the output gets mixed
together. This happens because there is no line-level locking for printf().
However, when reaching the barrier, each core prints a line that prints out
properly. This happens because all the other cores are busy waiting for the
barrier, effectively creating a mutual exclusion for the core that writes
to stdout.

By default, the demo program uses a mutex to control access to stdout while
the program is printing out some other things, so you will see the output
printed in order. To disable the mutex, rebuild the demo with USE_MUTEX set
to zero. Now you will see stdout output from all the cores being mixed up
together.

For this example, the BReset inputs of cores 1-3 are tied to a memory 
mapped register so that core0 can control when the other cores come out
of reset. This ensures that core0 can complete system init before the
other cores start running.

The example also provides a modified MPU mapping to create the shared
uncached region. If you need the shared uncached region in your system,
you will have to modify the MPU map to match your system's memory map.


System Layout
-------------

The XTSC system model constructed for this example looks like this:

    ---------    ---------    ---------    ---------
    | core0 |    | core1 |    | core2 |    | core3 |
    ---------    ---------    ---------    ---------
        |            |            |            |
        |            |            |            |
   ----------        |            |            |
   | router |        |            |            |
   ----------        |      ______|            |
     |     |         |     |                   |
  -------- |        -------------______________|
  | mmio | ---------|  arbiter  |
  --------          -------------
                         |
                    -----------------
                    | system memory |
                    -----------------

The MMIO block maps the BReset inputs of cores 1-3 into a memory mapped
register which core0 can write to release the other cores from reset.

Two templates are provided with this example, one for LX cores and one for
NX cores. The bus connectivity details are different for LX vs NX, but other
than that they are the same. For more details on XTSC usage and creating
system models with it refer to the XTSC Users Guide.


Creating a custom LSP with uncached memory region
-------------------------------------------------

The "sample_lsp" included here is an example of the kind of LSP that
will be needed if uncached memory is required for shared data. For your
configuration, you need to create an LSP by following these steps:

  - Make a copy of the "sim-mc" LSP included with your configuration.
    Give it a new name, e.g. "mylsp".

      xt-regenlsps -mlsp=sim-mc -b mylsp

  - Edit the memmap.xmm file in "mylsp" and make the following changes.
    Refer to the file sample_lsp/memmap.xmm as an example.

    1) Comment out the line starting with "PLACE SECTIONS(.xtos.lock)"

    2) Locate the memory segment that contains the .data section (in
       sample_lsp/memmap.xmm this is sram11).

    3) Add a new segment before this segment and rename the two as
       needed. Size the new segment to accommodate the sections that will
       go into it (start with a guess, say 16KB. If too small, the linker
       will report an error). If the original segment contains one of the
       exception vectors, you may need to split it into 3 segments (see
       below). Also, the segment start and size must comply with MPU region
       size and alignment requirements.

    4) Place the following sections into this new segment - .xtos.lock,
       .clib.data, .clib.bss. Remove these sections as needed from the
       existing segment where they were originally placed.

For example, the original LSP from which "sample_lsp" was derived contained
this segment:

 sram9 : F : 0x600003c0 - 0x63ffffff :  STACK :  HEAP : .DoubleExceptionVector.text
    .sram.rodata .rodata .sram.literal .literal .sram.text .text .sram.data
    .clib.data .data .xtos.lock .sram.bss .clib.bss .bss;

This was split into 3 segments:

 sram9 : F : 0x600003c0 - 0x60000fff : .DoubleExceptionVector.text;
 sram10 : F : 0x60001000 - 0x60007fff : .xtos.lock .clib.data .clib.bss;
 sram11 : F : 0x60008000 - 0x63ffffff : STACK : HEAP : .sram.rodata .rodata
    .sram.literal .literal .sram.text .text .sram.data .data .sram.bss .bss;

The new segment "sram10" contains the global data that will be mapped uncached.
Now you will have to modify the MPU setup table in the example to map just this
one segment uncached. Keep MPU region size and alignment requirements in mind.


Building and running
--------------------

To build, simply type "xt-make all" in this directory, assuming the Xtensa tools
are in your path and the Xtensa environment has been set up properly. The makefile
uses the sim-mc LSP by default. If you need to use your own custom LSP, edit the
makefile to set LSP=>your LSP name>, or override from the command line:

    xt-make all LSP=./mylsp

To run the example, type "xt-make run". To clean the build type "xt-make clean".


-End-

