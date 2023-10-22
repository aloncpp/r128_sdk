/* This linker script generated from xt-genldscripts.tpp for LSP /home1/xulu/work/r128-v0.6/lichee/dsp/arch/sun20iw2/lsp/dsp0/r128s1 */
/* Linker Script for default link */
MEMORY
{
  extra_mem_0_seg :                   	org = 0x0C600000, len = 0x660
  extra_mem_1_seg :                   	org = 0x0C600660, len = 0x9A0
  extra_mem_2_seg :                   	org = 0x0C601000, len = 0x17C
  extra_mem_3_seg :                   	org = 0x0C60117C, len = 0x20
  extra_mem_4_seg :                   	org = 0x0C60119C, len = 0x20
  extra_mem_5_seg :                   	org = 0x0C6011BC, len = 0x20
  extra_mem_6_seg :                   	org = 0x0C6011DC, len = 0x20
  extra_mem_7_seg :                   	org = 0x0C6011FC, len = 0x20
  extra_mem_8_seg :                   	org = 0x0C60121C, len = 0x20
  extra_mem_9_seg :                   	org = 0x0C60123C, len = 0x1C4
  extra_mem_10_seg :                  	org = 0x0C601402, len = 0x1FEBFE
}

PHDRS
{
  extra_mem_0_phdr PT_LOAD;
  extra_mem_1_phdr PT_LOAD;
  extra_mem_2_phdr PT_LOAD;
  extra_mem_3_phdr PT_LOAD;
  extra_mem_4_phdr PT_LOAD;
  extra_mem_5_phdr PT_LOAD;
  extra_mem_6_phdr PT_LOAD;
  extra_mem_7_phdr PT_LOAD;
  extra_mem_8_phdr PT_LOAD;
  extra_mem_9_phdr PT_LOAD;
  extra_mem_10_phdr PT_LOAD;
  extra_mem_10_bss_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(_ResetVector)


/*  Memory boundary addresses:  */
_memmap_mem_extra_mem_start = 0xc600000;
_memmap_mem_extra_mem_end   = 0xc800000;

/*  Memory segment boundary addresses:  */
_memmap_seg_extra_mem_0_start = 0xc600000;
_memmap_seg_extra_mem_0_max   = 0xc600660;
_memmap_seg_extra_mem_1_start = 0xc600660;
_memmap_seg_extra_mem_1_max   = 0xc601000;
_memmap_seg_extra_mem_2_start = 0xc601000;
_memmap_seg_extra_mem_2_max   = 0xc60117c;
_memmap_seg_extra_mem_3_start = 0xc60117c;
_memmap_seg_extra_mem_3_max   = 0xc60119c;
_memmap_seg_extra_mem_4_start = 0xc60119c;
_memmap_seg_extra_mem_4_max   = 0xc6011bc;
_memmap_seg_extra_mem_5_start = 0xc6011bc;
_memmap_seg_extra_mem_5_max   = 0xc6011dc;
_memmap_seg_extra_mem_6_start = 0xc6011dc;
_memmap_seg_extra_mem_6_max   = 0xc6011fc;
_memmap_seg_extra_mem_7_start = 0xc6011fc;
_memmap_seg_extra_mem_7_max   = 0xc60121c;
_memmap_seg_extra_mem_8_start = 0xc60121c;
_memmap_seg_extra_mem_8_max   = 0xc60123c;
_memmap_seg_extra_mem_9_start = 0xc60123c;
_memmap_seg_extra_mem_9_max   = 0xc601400;
_memmap_seg_extra_mem_10_start = 0xc601402;
_memmap_seg_extra_mem_10_max   = 0xc800000;

_rom_store_table = 0;
PROVIDE(_memmap_reset_vector = 0xc600660);
PROVIDE(_memmap_vecbase_reset = 0xc601000);
/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x00000001;
_memmap_cacheattr_wt_base = 0x00000003;
_memmap_cacheattr_bp_base = 0x00000004;
_memmap_cacheattr_unused_mask = 0xFFFFFFF0;
_memmap_cacheattr_wb_trapnull = 0x44444441;
_memmap_cacheattr_wba_trapnull = 0x44444441;
_memmap_cacheattr_wbna_trapnull = 0x44444442;
_memmap_cacheattr_wt_trapnull = 0x44444443;
_memmap_cacheattr_bp_trapnull = 0x44444444;
_memmap_cacheattr_wb_strict = 0x00000001;
_memmap_cacheattr_wt_strict = 0x00000003;
_memmap_cacheattr_bp_strict = 0x00000004;
_memmap_cacheattr_wb_allvalid = 0x44444441;
_memmap_cacheattr_wt_allvalid = 0x44444443;
_memmap_cacheattr_bp_allvalid = 0x44444444;
_memmap_region_map = 0x00000001;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wb_trapnull);

SECTIONS
{

  .oemhead.text : ALIGN(4)
  {
    _oemhead_text_start = ABSOLUTE(.);
    KEEP (*(.oemhead.text))
    . = ALIGN (4);
    _oemhead_text_end = ABSOLUTE(.);
  } >extra_mem_0_seg :extra_mem_0_phdr

  .oemhead.literal : ALIGN(4)
  {
    _oemhead_literal_start = ABSOLUTE(.);
    *(.oemhead.literal)
    . = ALIGN (4);
    _oemhead_literal_end = ABSOLUTE(.);
    _memmap_seg_extra_mem_0_end = ALIGN(0x8);
  } >extra_mem_0_seg :extra_mem_0_phdr


  .ResetVector.text : ALIGN(4)
  {
    _ResetVector_text_start = ABSOLUTE(.);
    KEEP (*(.ResetVector.text))
    . = ALIGN (4);
    _ResetVector_text_end = ABSOLUTE(.);
  } >extra_mem_1_seg :extra_mem_1_phdr

  .ResetHandler.text : ALIGN(4)
  {
    _ResetHandler_text_start = ABSOLUTE(.);
    *(.ResetHandler.literal .ResetHandler.text)
    . = ALIGN (4);
    _ResetHandler_text_end = ABSOLUTE(.);
    _memmap_seg_extra_mem_1_end = ALIGN(0x8);
  } >extra_mem_1_seg :extra_mem_1_phdr


  .WindowVectors.text : ALIGN(4)
  {
    _WindowVectors_text_start = ABSOLUTE(.);
    KEEP (*(.WindowVectors.text))
    . = ALIGN (4);
    _WindowVectors_text_end = ABSOLUTE(.);
  } >extra_mem_2_seg :extra_mem_2_phdr

  .Level2InterruptVector.literal : ALIGN(4)
  {
    _Level2InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level2InterruptVector.literal)
    . = ALIGN (4);
    _Level2InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_extra_mem_2_end = ALIGN(0x8);
  } >extra_mem_2_seg :extra_mem_2_phdr


  .Level2InterruptVector.text : ALIGN(4)
  {
    _Level2InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level2InterruptVector.text))
    . = ALIGN (4);
    _Level2InterruptVector_text_end = ABSOLUTE(.);
  } >extra_mem_3_seg :extra_mem_3_phdr

  .Level3InterruptVector.literal : ALIGN(4)
  {
    _Level3InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level3InterruptVector.literal)
    . = ALIGN (4);
    _Level3InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_extra_mem_3_end = ALIGN(0x8);
  } >extra_mem_3_seg :extra_mem_3_phdr


  .Level3InterruptVector.text : ALIGN(4)
  {
    _Level3InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level3InterruptVector.text))
    . = ALIGN (4);
    _Level3InterruptVector_text_end = ABSOLUTE(.);
  } >extra_mem_4_seg :extra_mem_4_phdr

  .DebugExceptionVector.literal : ALIGN(4)
  {
    _DebugExceptionVector_literal_start = ABSOLUTE(.);
    *(.DebugExceptionVector.literal)
    . = ALIGN (4);
    _DebugExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_extra_mem_4_end = ALIGN(0x8);
  } >extra_mem_4_seg :extra_mem_4_phdr


  .DebugExceptionVector.text : ALIGN(4)
  {
    _DebugExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DebugExceptionVector.text))
    . = ALIGN (4);
    _DebugExceptionVector_text_end = ABSOLUTE(.);
  } >extra_mem_5_seg :extra_mem_5_phdr

  .NMIExceptionVector.literal : ALIGN(4)
  {
    _NMIExceptionVector_literal_start = ABSOLUTE(.);
    *(.NMIExceptionVector.literal)
    . = ALIGN (4);
    _NMIExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_extra_mem_5_end = ALIGN(0x8);
  } >extra_mem_5_seg :extra_mem_5_phdr


  .NMIExceptionVector.text : ALIGN(4)
  {
    _NMIExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.NMIExceptionVector.text))
    . = ALIGN (4);
    _NMIExceptionVector_text_end = ABSOLUTE(.);
  } >extra_mem_6_seg :extra_mem_6_phdr

  .KernelExceptionVector.literal : ALIGN(4)
  {
    _KernelExceptionVector_literal_start = ABSOLUTE(.);
    *(.KernelExceptionVector.literal)
    . = ALIGN (4);
    _KernelExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_extra_mem_6_end = ALIGN(0x8);
  } >extra_mem_6_seg :extra_mem_6_phdr


  .KernelExceptionVector.text : ALIGN(4)
  {
    _KernelExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.KernelExceptionVector.text))
    . = ALIGN (4);
    _KernelExceptionVector_text_end = ABSOLUTE(.);
  } >extra_mem_7_seg :extra_mem_7_phdr

  .UserExceptionVector.literal : ALIGN(4)
  {
    _UserExceptionVector_literal_start = ABSOLUTE(.);
    *(.UserExceptionVector.literal)
    . = ALIGN (4);
    _UserExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_extra_mem_7_end = ALIGN(0x8);
  } >extra_mem_7_seg :extra_mem_7_phdr


  .UserExceptionVector.text : ALIGN(4)
  {
    _UserExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.UserExceptionVector.text))
    . = ALIGN (4);
    _UserExceptionVector_text_end = ABSOLUTE(.);
  } >extra_mem_8_seg :extra_mem_8_phdr

  .DoubleExceptionVector.literal : ALIGN(4)
  {
    _DoubleExceptionVector_literal_start = ABSOLUTE(.);
    *(.DoubleExceptionVector.literal)
    . = ALIGN (4);
    _DoubleExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_extra_mem_8_end = ALIGN(0x8);
  } >extra_mem_8_seg :extra_mem_8_phdr


  .DoubleExceptionVector.text : ALIGN(4)
  {
    _DoubleExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DoubleExceptionVector.text))
    . = ALIGN (4);
    _DoubleExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_extra_mem_9_end = ALIGN(0x8);
  } >extra_mem_9_seg :extra_mem_9_phdr


  .extra_mem.rodata : ALIGN(4)
  {
    _extra_mem_rodata_start = ABSOLUTE(.);
    *(.extra_mem.rodata)
    . = ALIGN (4);
    _extra_mem_rodata_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .clib.rodata : ALIGN(4)
  {
    _clib_rodata_start = ABSOLUTE(.);
    *(.clib.rodata)
    . = ALIGN (4);
    _clib_rodata_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .rtos.rodata : ALIGN(4)
  {
    _rtos_rodata_start = ABSOLUTE(.);
    *(.rtos.rodata)
    . = ALIGN (4);
    _rtos_rodata_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .rodata : ALIGN(4)
  {
    _rodata_start = ABSOLUTE(.);
    *(.rodata)
    *(SORT(.rodata.sort.*))
    KEEP (*(SORT(.rodata.keepsort.*) .rodata.keep.*))
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE__ = ABSOLUTE(.);
    KEEP (*(.xt_except_table))
    KEEP (*(.gcc_except_table))
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    KEEP (*(.eh_frame))
    /*  C++ constructor and destructor tables, properly ordered:  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    /*  C++ exception handlers table:  */
    __XT_EXCEPTION_DESCS__ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    . = ALIGN(4);		/* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = ABSOLUTE(.);
    . = ALIGN (4);
    _rodata_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .FSymTab : ALIGN(4)
  {
    _FSymTab_start = ABSOLUTE(.);
    KEEP (*(.FSymTab))
    . = ALIGN (4);
    _FSymTab_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .stubTab : ALIGN(4)
  {
    _stubTab_start = ABSOLUTE(.);
    *(.stubTab)
    . = ALIGN (4);
    _stubTab_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .extra_mem.text : ALIGN(4)
  {
    _extra_mem_text_start = ABSOLUTE(.);
    *(.extra_mem.literal .extra_mem.text)
    . = ALIGN (4);
    _extra_mem_text_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    *(.init.literal)
    KEEP(*(.init))
    *(.literal.sort.* SORT(.text.sort.*))
    KEEP (*(.literal.keepsort.* SORT(.text.keepsort.*) .literal.keep.* .text.keep.* .literal.*personality* .text.*personality*))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    . = ALIGN (4);
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >extra_mem_10_seg :extra_mem_10_phdr

  .clib.text : ALIGN(4)
  {
    _clib_text_start = ABSOLUTE(.);
    *(.clib.literal .clib.text)
    . = ALIGN (4);
    _clib_text_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .rtos.text : ALIGN(4)
  {
    _rtos_text_start = ABSOLUTE(.);
    *(.rtos.literal .rtos.text)
    . = ALIGN (4);
    _rtos_text_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .clib.data : ALIGN(4)
  {
    _clib_data_start = ABSOLUTE(.);
    *(.clib.data)
    . = ALIGN (4);
    _clib_data_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .clib.percpu.data : ALIGN(4)
  {
    _clib_percpu_data_start = ABSOLUTE(.);
    *(.clib.percpu.data)
    . = ALIGN (4);
    _clib_percpu_data_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .rtos.percpu.data : ALIGN(4)
  {
    _rtos_percpu_data_start = ABSOLUTE(.);
    *(.rtos.percpu.data)
    . = ALIGN (4);
    _rtos_percpu_data_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .rtos.data : ALIGN(4)
  {
    _rtos_data_start = ABSOLUTE(.);
    *(.rtos.data)
    . = ALIGN (4);
    _rtos_data_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .extra_mem.data : ALIGN(4)
  {
    _extra_mem_data_start = ABSOLUTE(.);
    *(.extra_mem.data)
    . = ALIGN (4);
    _extra_mem_data_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .data : ALIGN(4)
  {
    _data_start = ABSOLUTE(.);
    *(.data)
    *(SORT(.data.sort.*))
    KEEP (*(SORT(.data.keepsort.*) .data.keep.*))
    *(.data.*)
    *(.gnu.linkonce.d.*)
    KEEP(*(.gnu.linkonce.d.*personality*))
    *(.data1)
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)
    KEEP(*(.jcr))
    *(__llvm_prf_cnts)
    *(__llvm_prf_data)
    *(__llvm_prf_vnds)
    . = ALIGN (4);
    _data_end = ABSOLUTE(.);
  } >extra_mem_10_seg :extra_mem_10_phdr

  .bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.);
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    *(.bss)
    *(SORT(.bss.sort.*))
    KEEP (*(SORT(.bss.keepsort.*) .bss.keep.*))
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    *(.clib.bss)
    *(.clib.percpu.bss)
    *(.rtos.percpu.bss)
    *(.rtos.bss)
    *(.extra_mem.bss)
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
    _end = ALIGN(0x8);
    PROVIDE(end = ALIGN(0x8));
    _stack_sentry = ALIGN(0x8);
    _memmap_seg_extra_mem_10_end = ALIGN(0x8);
  } >extra_mem_10_seg :extra_mem_10_bss_phdr

  PROVIDE(__stack = 0xc800000);
  _heap_sentry = 0xc800000;

  _memmap_mem_extra_mem_max = ABSOLUTE(.);

  .debug  0 :  { *(.debug) }
  .line  0 :  { *(.line) }
  .debug_srcinfo  0 :  { *(.debug_srcinfo) }
  .debug_sfnames  0 :  { *(.debug_sfnames) }
  .debug_aranges  0 :  { *(.debug_aranges) }
  .debug_pubnames  0 :  { *(.debug_pubnames) }
  .debug_info  0 :  { *(.debug_info) }
  .debug_abbrev  0 :  { *(.debug_abbrev) }
  .debug_line  0 :  { *(.debug_line) }
  .debug_frame  0 :  { *(.debug_frame) }
  .debug_str  0 :  { *(.debug_str) }
  .debug_loc  0 :  { *(.debug_loc) }
  .debug_macinfo  0 :  { *(.debug_macinfo) }
  .debug_weaknames  0 :  { *(.debug_weaknames) }
  .debug_funcnames  0 :  { *(.debug_funcnames) }
  .debug_typenames  0 :  { *(.debug_typenames) }
  .debug_varnames  0 :  { *(.debug_varnames) }
  .xt.insn 0 :
  {
    KEEP (*(.xt.insn))
    KEEP (*(.gnu.linkonce.x.*))
  }
  .xt.prop 0 :
  {
    KEEP (*(.xt.prop))
    KEEP (*(.xt.prop.*))
    KEEP (*(.gnu.linkonce.prop.*))
  }
  .xt.lit 0 :
  {
    KEEP (*(.xt.lit))
    KEEP (*(.xt.lit.*))
    KEEP (*(.gnu.linkonce.p.*))
  }
  .debug.xt.callgraph 0 :
  {
    KEEP (*(.debug.xt.callgraph .debug.xt.callgraph.* .gnu.linkonce.xt.callgraph.*))
  }
  .note.gnu.build-id 0 :
  {
    KEEP(*(.note.gnu.build-id))
  }
}

