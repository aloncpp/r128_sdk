#!/bin/bash
#set -x

WD=$PWD
HOSTOS=$(uname -o 2>/dev/null || echo "Other")

unset CPUOPT
if [ "X${1}" == "X-j" ]; then
  shift
  CPUOPT="-j $1"
  shift
fi

TESTLIST=$1

if [ -z "${PATH_ORIG}" ]; then
  export PATH_ORIG="${PATH}"
fi

rm -f armlist.dat
if [ "X${HOSTOS}" == "XCygwin" ]; then
#  TOOLCHAIN_BIN="/cygdrive/c/Program Files (x86)/GNU Tools ARM Embedded/4.9 2015q2/bin"
  TOOLCHAIN_BIN="/cygdrive/c/Program Files (x86)/GNU Tools ARM Embedded/8 2019-q3-update/bin"
  cat armlist.template | sed -e "s/_EABIx/_EABIW/g" >armlist.dat
  ENVOPT="-w -c"
else
  TOOLCHAIN_BIN="/usr/bin/gcc-arm-none-eabi-9-2019-q4-major/bin"
  cat armlist.template | sed -e "s/_EABIx/_EABIL/g" >armlist.dat
  ENVOPT=-l
fi

export PATH="${TOOLCHAIN_BIN}:/sbin:/usr/sbin:${PATH_ORIG}"

if [ -z "$TESTLIST" ]; then
  TESTLIST=armlist.dat
fi

TESTBUILD=./testbuild.sh
if [ ! -x "$TESTBUILD" ]; then
  echo "Help!!! I can't find testbuild.sh"
  exit 1
fi

$TESTBUILD -e -Werror $CPUOPT $ENVOPT $WD/$TESTLIST 1>$WD/armtest.log 2>&1
rm -f $WD/armlist.dat
