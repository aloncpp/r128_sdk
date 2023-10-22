#!/bin/bash

WD=$PWD

TESTLIST=$1
if [ -z "$TESTLIST" ]; then
  TESTLIST=mipslist.dat
fi

if [ -z "${PATH_ORIG}" ]; then
  export PATH_ORIG="${PATH}"
fi

# Careful: Toolchain path needs to go at the end to avoid picking up MSYS functions instead of Cygwin

export TOOLCHAIN_PREBIN=
export TOOLCHAIN_POSTBIN=:"/cygdrive/c/pinguino-11/compilers/p32/bin"
export PATH="${TOOLCHAIN_PREBIN}${PIC32TOOL_DIR}:/sbin:/usr/sbin:${PATH_ORIG}${TOOLCHAIN_POSTBIN}"

TESTBUILD=./testbuild.sh
if [ ! -x "$TESTBUILD" ]; then
  echo "Help!!! I can't find testbuild.sh"
  exit 1
fi

# NOTE: Some linker scripts contain this line: OUTPUT_ARCH(pic32mx)
# This will cause a linking failure in the very last steps and must be ignored

$TESTBUILD -c $WD/$TESTLIST 1>$WD/mipstest.log 2>&1
