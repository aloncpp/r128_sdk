OUTPUT_ARCH(arm)
ENTRY(standby_main)

MEMORY
{
	/* 64k ram area for code  */
	ram     : ORIGIN = CONFIG_ARCH_PM_FIRMWARE_BASE, LENGTH = CONFIG_ARCH_PM_FIRMWARE_SIZE
}

SECTIONS
{
	.paras :
	{
		_standby_paras_start = .;
		*( .standby_head )
		. = ALIGN(16);
		_standby_paras_end = .;
	} > ram

	. = ALIGN(8);
	.text :
	{
		_standby_code_start = .;
		main.o(.text)
		*(.text)
		*(.rodata.*)
	} > ram

	.data :
	{
		*(.data)
	} > ram

	.ARM.exidx :
	{
		*(.ARM.exidx*)
		*(.gnu.linkonce.armexidx.*)
	} > ram

	_standby_code_end = . ;

	. = ALIGN(4);
	.bss :
	{
		_standby_bss_start = . ;
		*(.sbss)
		*(.scommon)
		*(.bss)
		*(COMMON)
	} > ram
	_standby_bss_end = .;

	. = ALIGN(8);
	_standby_stack_limit = .;
	_standby_stack_base  = ALIGN(CONFIG_ARCH_PM_FIRMWARE_BASE + CONFIG_ARCH_PM_FIRMWARE_SIZE);
}

