#!/bin/bash
function build_usage()
{
    printf "Usage: ./build.sh [args]
    ./build.sh             - default build all
    ./build.sh menuconfig  - build defconfig & save defconfig
    ./build.sh clean       - clean project
    ./build.sh tools       - install tools of xcc
"
    return 0
}

function xcc_help()
{
    printf "Invoke . build/envsetup.sh from your shell to add the following functions to your environment:
    env_xcc             - use xcc env
    env_host            - use host env
"
    return 0
}

function build_help()
{
    printf "Invoke . build/envsetup.sh from your shell to add the following functions to your environment:
    croot               - Changes directory to the top of the tree.
    carch               - Changes directory to the arch
    clsp            	- Changes directory to the lsp
    cout                - Changes directory to the out
    cconfig             - Changes directory to the defconfig path
    chal                - Changes directory to the rtos-hal
    doobjdump           - run xt-objdump to decompile the dsp elf
    dogenlds            - run xt-genldscripts to regenerate the linker scripts
    callstack           - run backtrace.sh to get callstack infortion
    mdsp                - run ./build.sh with arguments
"
    return 0
}

function load_config()
{
	local cfgkey=$1
	local cfgfile=$2
	local defval=$3
	local val=""

	[ -f "$cfgfile" ] && val="$(sed -n "/^\s*export\s\+$cfgkey\s*=/h;\${x;p}" $cfgfile | sed -e 's/^[^=]\+=//g' -e 's/^\s\+//g' -e 's/\s\+$//g')"
	eval echo "${val:-"$defval"}"
}


function printconfig()
{
	cat $LICHEE_TOP_DIR/.buildconfig
}

function env_xcc()
{
	local dkey="LICHEE_TOP_DIR"
	local dval=$(load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	PATH=${LICHEE_CUR_PATH}
}

function env_host()
{
	local dkey="LICHEE_HOST_PATH"
	local dval=$(load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	PATH=${LICHEE_HOST_PATH}
}

function croot()
{
	local dkey="LICHEE_TOP_DIR"
	local dval=$(load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval
}

function cout()
{
	local dkey="LICHEE_CHIP_OUT_DIR"
	local dval=$(load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval
}

function cconfig()
{
	local dkey="LICHEE_CHIP_CORE_DIR"
	local dval_0=$(load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval_0" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval_0
}

function carch()
{
	local dkey="LICHEE_CHIP_ARCH_DIR"
	local dval=$(load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval
}

function clsp()
{
	local dkey="LICHEE_CHIP_LSP_DIR"
	local dval=$(load_config $dkey $LICHEE_TOP_DIR/.buildconfig)
	[ -z "$dval" ] && echo "ERROR: $dkey not set in .buildconfig" && return 1
	cd $dval
}

function chal()
{
	cd $LICHEE_TOP_DIR/../rtos-hal/
}

function doobjdump()
{
	local BUILD_CONFIG=$LICHEE_TOP_DIR/.buildconfig
	local TOOLCHAIN_DIR=$(load_config XTENSA_TOOLS_DIR $BUILD_CONFIG)
	local XTENSA_CORE=$(load_config XTENSA_CORE $BUILD_CONFIG)
	local XTENSA_SYSTEM=$(load_config XTENSA_SYSTEM $BUILD_CONFIG)
	local IC_VERSION=$(load_config LICHEE_IC $BUILD_CONFIG)
	local DSP_CORE=$(load_config LICHEE_DSP_CORE $BUILD_CONFIG)
	local OUT_DIR=$(load_config LICHEE_CHIP_OUT_DIR $BUILD_CONFIG)
	local CHIP_BOARD=$(load_config LICHEE_CHIP_BOARD $BUILD_CONFIG)
	$TOOLCHAIN_DIR/bin/xt-objdump --xtensa-core=$XTENSA_CORE --xtensa-system=$XTENSA_SYSTEM -S $OUT_DIR/${IC_VERSION}_${DSP_CORE}_${CHIP_BOARD}.elf | less
}

segment_name_string=(
".version_table"
".resource_table"
".FSymTab"
)

xmm_key_string=(
"USE_PHYS_ADDR_FOR_LMA=true"
)

function dogenlds()
{
	local BUILD_CONFIG=$LICHEE_TOP_DIR/.buildconfig
	local TOOLCHAIN_DIR=$(load_config XTENSA_TOOLS_DIR $BUILD_CONFIG)
	local XTENSA_CORE=$(load_config XTENSA_CORE $BUILD_CONFIG)
	local XTENSA_SYSTEM=$(load_config XTENSA_SYSTEM $BUILD_CONFIG)
	local DSP_CORE=$(load_config LICHEE_DSP_CORE $BUILD_CONFIG)
	local LSP_PATH=$(load_config LICHEE_CHIP_LSP_DIR $BUILD_CONFIG)
	local XMM_FILE=$LSP_PATH/memmap.xmm
	local XLD_FILE=$LSP_PATH/memmap.xld
	local LDS_FILE=$LSP_PATH/ldscripts
	local val_list

	if [ ! -f $XMM_FILE  ];then
		printf "The $XMM_FILE could not be found\n"
		return 1;
	fi

	if [ ! -f $XLD_FILE  ];then
		printf "The $XLD_FILE could not be found\n"
		return 1;
	fi

	rm -rf $LDS_FILE
	val_list="${xmm_key_string[@]}"
	for val in $val_list; do
		if [ -z "`grep -nwr "${val}" $XMM_FILE`"  ];then
			printf "${val} could not be found in $XMM_FILE\n"
		fi
	done
	val_list="${segment_name_string[@]}"
	for val in $val_list; do
		if [ -z "`grep -nwr "${val}" $XMM_FILE`"  ];then
			printf "${val} could not be found in $XMM_FILE\n"
		fi
	done

	$TOOLCHAIN_DIR/bin/xt-genldscripts --xtensa-core=$XTENSA_CORE --xtensa-system=$XTENSA_SYSTEM -b $LSP_PATH

	val_list="${segment_name_string[@]}"
	for val in $val_list; do
		sed  -i "s/*(${val})/KEEP (*(${val}))/g" $LDS_FILE/*
	done

	printf "Rebuild lsp of $LSP_PATH successfully\n"

}

function callstack()
{
	local BUILD_CONFIG=$LICHEE_TOP_DIR/.buildconfig
	local IC_VERSION=$(load_config LICHEE_IC $BUILD_CONFIG)
	local CHIP_BOARD=$(load_config LICHEE_CHIP_BOARD $BUILD_CONFIG)
	local DSP_CORE=$(load_config LICHEE_DSP_CORE $BUILD_CONFIG)
	local OUT_DIR=$(load_config LICHEE_CHIP_OUT_DIR $BUILD_CONFIG)

	$LICHEE_TOP_DIR/backtrace.sh $1 $OUT_DIR/${IC_VERSION}_${DSP_CORE}_${CHIP_BOARD}.elf
}

function mdsp()
{
	$LICHEE_TOP_DIR/build.sh $1
}

if [ ! -f build/envsetup.sh ] || [ ! -f build.sh ]; then
	echo "MUST do this in LICHEE_TOP_DIR."
else
    if [ ! -f .buildconfig ]; then
	./build.sh config
    fi

	export LICHEE_TOP_DIR=$(pwd)
    . $LICHEE_TOP_DIR/.buildconfig
	build_usage
	build_help
	xcc_help
    echo "run envsetup finish."
fi
