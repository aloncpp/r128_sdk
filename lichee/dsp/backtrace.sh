#!/bin/bash

log=$1
elf_file=$2

ADDR2LINE=xt-addr2line

function load_config()
{
    local cfgkey=$1
    local cfgfile=$2
    local defval=$3
    local val=""

    [ -f "$cfgfile" ] && val="$(sed -n "/^\s*export\s\+$cfgkey\s*=/h;\${x;p}" $cfgfile | sed -e 's/^[^=]\+=//g' -e 's/^\s\+//g' -e 's/\s\+$//g')"
    eval echo "${val:-"$defval"}"
}

BUILD_CONFIG=.buildconfig
TOOLCHAIN_DIR=$(load_config XTENSA_TOOLS_DIR $BUILD_CONFIG)
XTENSA_CORE=$(load_config XTENSA_CORE $BUILD_CONFIG)
XTENSA_SYSTEM=$(load_config XTENSA_SYSTEM $BUILD_CONFIG)

echo ""

for line in $(cat $log)
do
	line=`echo $line | tr -d "\n"`
	line=`echo $line | tr -d "\r"`
	if [ -z $line ];then
		continue;
	fi
	result=`$TOOLCHAIN_DIR/bin/$ADDR2LINE --xtensa-core=$XTENSA_CORE --xtensa-system=$XTENSA_SYSTEM -f -e $elf_file -a $line`
	echo $result
	echo ""
done
