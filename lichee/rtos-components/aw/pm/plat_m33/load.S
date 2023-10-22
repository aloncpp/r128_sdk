
  .globl _standby_bin_start
  .globl _standby_bin_end

  .align 2
_standby_bin_start:
  .incbin "components/common/aw/pm/plat_m33/firmware/standby.bin"
  .align 2
_standby_bin_end:
  .align 2
