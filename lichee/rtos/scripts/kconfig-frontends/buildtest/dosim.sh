#!/bin/bash

WD=$PWD
HOSTOS=$(uname -o 2>/dev/null || echo "Other")

unset CPUOPT
if [ "X${1}" == "X-j" ]; then
  shift
  CPUOPT="-j $1"
  shift
fi

TESTLIST=$1
if [ -z "$TESTLIST" ]; then
  TESTLIST=simlist.dat
fi

if [ -z "${PATH_ORIG}" ]; then
  export PATH_ORIG="${PATH}"
fi

if [ "X${HOSTOS}" == "XCygwin" ]; then
  ENVOPT=-c
else
  ENVOPT=-l
fi

TESTBUILD=./testbuild.sh
if [ ! -x "$TESTBUILD" ]; then
  echo "Help!!! I can't find testbuild.sh"
  exit 1
fi

$TESTBUILD $CPUOPT $ENVOPT $WD/$TESTLIST 1>$WD/simtest.log 2>&1
