#!/bin/sh

ARCHLIST="arm avr hc mips misoc or1k renesas risc-v sim x86 xtensa z16 z80"
CHIPLIST="a1x am335x c5471 cxd56xx dm320 efm32 imx6 imxrt kinetis kl lc823450
 lpc17xx_40xx lpc214x lpc2378 lpc31xx lpc43xx lpc54xx max326xx moxart nrf52
 nuc1xx s32k1xx sam34 sama5 samd2l2 samd5e5 samv7 stm32 stm32f0l0g0 stm32f7 stm32h7
 stm32l4 str71x tiva tms570 xmc4 at32uc3 at90usb atmega mcs92s12ne64 pic32mx
 pic32mz lm32 mor1kx m32262f8 sh7032 gap8 nr5m100 sim qemu esp32 z16f2811
 ez80 z180 z8 z80"

FILE=$1
if [ -z "$FILE" ]; then
  FILE=$PWD/armlist.template
fi

cd ..
if [ ! -d nuttx ]; then
  cd ..
  if [ ! -d nuttx ]; then
    echo "ERROR:  Cannot find the nuttx/ directory"
  fi
fi

nuttx=$PWD/nuttx

LIST=`cat $FILE`
for line in $LIST; do
# firstch=${line:0:1}
  firstch=`echo $line | cut -c1-1`
  if [ "X$firstch" != "X#" ]; then
    # Parse the configuration spec

    configspec=`echo $line | cut -d',' -f1`
    board=`echo $configspec | cut -d':' -f1`
    config=`echo $configspec | cut -d':' -f2`

    # Detect the architecture of this board.

    for arch in ${ARCHLIST}; do
      for chip in ${CHIPLIST}; do
        if [ -f ${nuttx}/boards/${arch}/${chip}/${board}/Kconfig ]; then
          archdir=${arch}
          chipdir=${chip}
        fi
      done
    done

    if [ -z "${archdir}" ]; then
      echo "ERROR:  Architecture of ${board} not found"
    else
      path=$nuttx/boards/$archdir/$chipdir/$board/configs/$config/defconfig
      if [ ! -r $path ]; then
        echo "ERROR: $path does not exist"
      fi
    fi
  fi
done
