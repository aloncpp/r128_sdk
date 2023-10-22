function hmm() { 
cat <<EOF
Invoke ". envsetup.sh" from your shell to add the following functions to your environment:

== build project ==
- mboot:        Build boot0 and uboot, including uboot for nor.
- mboot0:       Just build boot0.
- muboot:       Build uboot, including uboot for nor.

== jump directory ==
- croot:    Jump to the top of the tree.
- cboot:    Jump to uboot.
- cboot0:   Jump to boot0.
- cbin:     Jump to uboot/boot0 bin directory.
- cconfigs: Jump to configs of target.
- cout:     Jump to out directory of target.
- cdsp:     Jump to dsp.
- chal:     Jump to rtos-hal.
- ccomponents: Jump to rtos-components.
- cbuild:   Jump to rtos build dir.
- cprojects:Jump to rtos projects dir.

Look at the source to view more functions. The complete list is:
EOF
    T=$(gettop)
    local A
    A=""
    for i in `cat $T/tools/scripts/envsetup.sh | sed -n "/^[ \t]*function /s/function \([a-z_]*\).*/\1/p" | sort | uniq`; do
      A="$A $i"
    done
    echo $A
}

function gettop()
{
    local TOPFILE=tools/scripts/envsetup.sh
    if [ -n "$RTOS_TOP" -a -f "$RTOS_TOP/$TOPFILE" ] ; then
        # The following circumlocution ensures we remove symlinks from TOP.
        (\cd $RTOS_TOP; PWD= /bin/pwd)
    else
        if [ -f $TOPFILE ] ; then
            # The following circumlocution (repeated below as well) ensures
            # that we record the true directory name and not one that is
            # faked up with symlink names.
            PWD= /bin/pwd
        else
            local here="${PWD}"
            while [ "${here}" != "/" ]; do
                if [ -f "${here}/${TOPFILE}" ]; then
                    (\cd ${here}; PWD= /bin/pwd)
                    break
                fi
                here="$(dirname ${here})"
            done
        fi
    fi
}

function getimgdir()
{
    local T=$(gettop)
    local IMG_DIR="${T}/lichee/rtos/build/${RTOS_PROJECT_NAME}/img"
    echo ${IMG_DIR}
}

function getbindir()
{
    if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]; then
        local BIN_DIR="board/${RTOS_TARGET_BOARD_PATH%_*}/bin"
    else
        local BIN_DIR="board/${RTOS_TARGET_BOARD_PATH}/bin"
    fi
    echo ${BIN_DIR}
}

function envsetup
{
    if [ "x$SHELL" != "x/bin/bash" ]; then
        case `ps -o command -p $$` in
            *bash*)
                ;;
            *)
                echo -n "WARNING: Only bash is supported, "
                echo "use of other shell would lead to erroneous results"
                ;;
        esac
    fi

    # check top of SDK
    if [ ! -f "${PWD}/tools/scripts/envsetup.sh" ]; then
        echo "ERROR: Please source envsetup.sh in the root of SDK"
        return -1
    else
        export RTOS_TOP="$(PWD= /bin/pwd)"
    fi

    export TARGET_BUILD_VARIANT=rtos

    if [ -d "${PWD}/lichee/nuttx" ]; then
        export TARGET_BUILD_RTOS=nuttx
    elif [ -d "${PWD}/lichee/freertos" ]; then
        export TARGET_BUILD_RTOS=freertos
    elif [ -d "${PWD}/lichee/rtos" ]; then
        export TARGET_BUILD_RTOS=freertos
    else
        export TARGET_BUILD_RTOS=unknown
    fi

    if [ x"${TARGET_BUILD_RTOS}" == x"nuttx" ]; then
        if [ "`echo $PATH | grep kconfig`"  != "" ]; then
            echo "Already prepare PATH for build ..."
        else
            echo "Prepare PATH for build ...";
            export PATH=$PATH:${PWD}/lichee/nuttx/tools/prebuilt/gcc-arm-melis-eabi-9-2019-q4-major/bin/;
            export PATH=$PATH:${PWD}/lichee/nuttx/tools/prebuilt/kconfig-frontends/bin/;
            export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${PWD}/lichee/nuttx/tools/prebuilt/kconfig-frontends/lib/
        fi
    elif [ x"${TARGET_BUILD_RTOS}" == x"freertos" ]; then
        get_all_projects
        complete -F _lunch lunch
    fi

    echo "Setup env done!"
    echo -e "Run \033[32mlunch_rtos\033[0m to select project"
}

function m
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    (\cd $T && mrtos $@)
}

function p
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    (\cd $T && pack $@)
}

function mp
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    (\cd $T && mrtos && pack $@)
}

# Build brandy(uboot,boot0,fes) if you want.
function build_boot()
{
	local T=$(gettop)
	local chip=${RTOS_TARGET_CHIP}
	local cmd=$1
	local o_option=$2
	local platform
	local bin_dir=$(getbindir)
	local special_config=""
	export LICHEE_FLASH="default"


	if [ "x$chip" = "x" ]; then
		echo "platform($RTOS_TARGET_PROJECT%%_*) not support"
		return 1
	fi

	if [ -f "$T/board/${RTOS_TARGET_BOARD_PATH%_*}/configs/BoardConfig.mk" ]; then
		special_config="$T/board/${RTOS_TARGET_BOARD_PATH%_*}/configs/BoardConfig.mk"
	elif [ -f "$T/board/${RTOS_TARGET_BOARD_PATH}/configs/BoardConfig.mk" ]; then
		special_config="$T/board/${RTOS_TARGET_BOARD_PATH}/configs/BoardConfig.mk"
	fi

	if [ x"$o_option" = x"uboot" ]; then
		platform=${chip}_rtos
		if [ x"${special_config}" != x"" ]; then
			platform=$(grep "LICHEE_BRANDY_DEFCONF" ${special_config} | awk -F "=" '{print $2}')
			platform=${platform%%_def*}
		fi
	else
		platform=${chip}
	fi

	\cd $T/lichee/brandy-2.0/
	mkdir -p $T/${bin_dir}

	if [ x"$o_option" == "xboot0" ]; then
		o_option=spl
	fi

	echo "build_boot platform:$platform o_option:$o_option"
	if [ x"$o_option" != "x" ]; then
		TARGET_BIN_DIR=${bin_dir} ./build.sh -p $platform -o $o_option
	else
		TARGET_BIN_DIR=${bin_dir} ./build.sh -p $platform
	fi
	if [ $? -ne 0 ]; then
		echo "$cmd stop for build error in brandy, Please check!"
		\cd - 1>/dev/null
		return 1
	fi
	\cd - 1>/dev/null
	echo "$cmd success!"
	return 0
}

function muboot
{
    (build_boot muboot uboot)
}

function mboot
{
    (build_boot muboot uboot)
    (build_boot mboot0 boot0)
}

function mboot0
{
    (build_boot mboot0 boot0)
}

pack_usage()
{
	printf "Usage: pack [-cCHIP] [-pPLATFORM] [-bBOARD] [-oOS] [-fPROJECT_PATH] [-gBOARD_PATH] [-s] [-m] [-w] [-i] [-h]
	-c CHIP (default: $chip)
	-p PLATFORM (default: $platform)
	-b BOARD (default: $board)
	-s pack firmware with signature
	-m pack dump firmware
	-w pack programmer firmware
	-i pack sys_partition.fex downloadfile img.tar.gz
	-h print this help message
	-f project path
	-g board path
"
}

function generate_image_header() {
	local T=$(gettop)
	local CCONFIG_DIR="$T/board/${RTOS_TARGET_BOARD_PATH%_*}/configs"

	local CC_SYMBOLS=""
	local ARM_DEFCONFIG="${T}/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH%_*}_m33/defconfig"
	local RV_DEFCONFIG="${T}/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH%_*}_c906/defconfig"

	local ARM_TFM=`grep "CONFIG_COMPONENTS_TFM=" ${ARM_DEFCONFIG} > /dev/null && echo 1 || echo 0`
	if [ "x$1" == "xsecure" -a "x$ARM_TFM" == "x0" ]; then
		echo "ERROR: please enable CONFIG_COMPONENTS_TFM and rebuild m33 before run 'pack -s'"
		return 1
	fi

	if [ "x$1" == "xnone" -a "x$ARM_TFM" == "x1" ]; then
		echo "ERROR: please disable CONFIG_COMPONENTS_TFM and rebuild m33 before run 'pack'"
		return 1
	fi

	local ARM_XIP=`grep "CONFIG_XIP=" ${ARM_DEFCONFIG} > /dev/null && echo 1 || echo 0`
	local RV_XIP=`grep "CONFIG_XIP=" ${RV_DEFCONFIG} > /dev/null && echo 1 || echo 0`
	local ARM_PSRAM=`grep "CONFIG_PSRAM=" ${ARM_DEFCONFIG} > /dev/null && echo 1 || echo 0`
	local ARM_PSRAM_LOAD_ADDR=`grep "CONFIG_PSRAM_START_ADDRESS=" ${ARM_DEFCONFIG} | awk -F= '{print $NF}'`
	local ARM_RECOVERY=`grep "CONFIG_COMPONENTS_AW_OTA_V2_RECOVERY=" ${RV_DEFCONFIG} > /dev/null && echo 1 || echo 0`
	local ARM_BOOT_RV=`grep "CONFIG_ARCH_ARMV8M_DEFAULT_BOOT_RISCV=" ${ARM_DEFCONFIG} > /dev/null && echo 1 || echo 0`
	local ARM_BOOT_DSP=`grep "CONFIG_ARCH_ARMV8M_DEFAULT_BOOT_DSP=" ${ARM_DEFCONFIG} > /dev/null && echo 1 || echo 0`
	local ARM_LOAD_ADDR=`grep "CONFIG_ARCH_START_ADDRESS=" ${ARM_DEFCONFIG} | awk -F= '{print $NF}'`
	local RV_LOAD_ADDR=`grep "CONFIG_ARCH_RISCV_START_ADDRESS=" ${ARM_DEFCONFIG} | awk -F= '{print $NF}'`
	local DSP_RUN_ADDR=`grep "CONFIG_ARCH_DSP_START_ADDRESS=" ${ARM_DEFCONFIG} | awk -F= '{print $NF}'`
	local DSP_LOAD_ADDR=`expr $(printf %d $DSP_RUN_ADDR) - 1632 | xargs printf 0x%08x`


	CC_SYMBOLS+="-DARM_TFM=${ARM_TFM} "
	CC_SYMBOLS+="-DARM_XIP=${ARM_XIP} "
	CC_SYMBOLS+="-DRV_XIP=${RV_XIP} "
	CC_SYMBOLS+="-DARM_PSRAM=${ARM_PSRAM} "
	CC_SYMBOLS+="-DARM_PSRAM_LOAD_ADDR=${ARM_PSRAM_LOAD_ADDR} "
	CC_SYMBOLS+="-DARM_RECOVERY=${ARM_RECOVERY} "
	CC_SYMBOLS+="-DARM_BOOT_RV=${ARM_BOOT_RV} "
	CC_SYMBOLS+="-DARM_BOOT_DSP=${ARM_BOOT_DSP} "
	CC_SYMBOLS+="-DARM_LOAD_ADDR=${ARM_LOAD_ADDR} "
	CC_SYMBOLS+="-DRV_LOAD_ADDR=${RV_LOAD_ADDR} "
	CC_SYMBOLS+="-DDSP_RUN_ADDR=${DSP_RUN_ADDR} "
	CC_SYMBOLS+="-DDSP_LOAD_ADDR=${DSP_LOAD_ADDR} "

	echo $CC_SYMBOLS

	if [ "$(expr substr $(uname -s) 1 6)" == "CYGWIN" ];then
		cd ${CCONFIG_DIR}
		rm -rf image_header.cfg
		${RTOS_BUILD_TOOLCHAIN}gcc -E -P -CC ${CC_SYMBOLS} -o image_header.cfg - < image_header.template.cfg
		sed -i '1i\/* -- Autogenerated! Do not edit!!! */\n' image_header.cfg
		cd -
	else
		rm -rf ${CCONFIG_DIR}/image_header.cfg
		${RTOS_BUILD_TOOLCHAIN}gcc -E -P -CC ${CC_SYMBOLS} -I${CCONFIG_DIR} -o ${CCONFIG_DIR}/image_header.cfg - < ${CCONFIG_DIR}/image_header.template.cfg
		sed -i '1i\/* -- Autogenerated! Do not edit!!! */\n' ${CCONFIG_DIR}/image_header.cfg
	fi

	return 0
}

function pack() {
	local T=$(gettop)
	local chip=${RTOS_TARGET_CHIP}
	local platform=rtos

        if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]; then
	    local project_path=${RTOS_TARGET_PROJECT_PATH%_*}
	    local board_path=${RTOS_TARGET_BOARD_PATH%_*}
	    local board=${RTOS_PROJECT_NAME%_*}
	else
	    local project_path=${RTOS_TARGET_PROJECT_PATH}
	    local board_path=${RTOS_TARGET_BOARD_PATH}
	    local board=${RTOS_PROJECT_NAME%_*}
	fi

	local debug=uart0
	local sigmode=none
	local securemode=none
	local mode=normal
	local programmer=none
	local tar_image=none
	local os=${TARGET_BUILD_RTOS}
	local hostos=linux
	unset OPTIND
	while getopts "dsvmwih" arg
	do
		case $arg in
			s)
				sigmode=secure
				;;
			v)
				securemode=secure
				;;
			m)
				mode=dump
				;;
			w)
				programmer=programmer
				;;
			i)
				tar_image=tar_image
				;;
			h)
				pack_usage
				return 0
				;;
			?)
			return 1
			;;
		esac
	done

	chip=${RTOS_TARGET_CHIP}
	if [ "x$chip" = "x" ]; then
		echo "platform($RTOS_PROJECT_NAME%%_*) not support"
		return
	fi

        if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]; then
		generate_image_header $sigmode
		[ $? -ne 0 ] && return
	fi

	$T/tools/scripts/pack_img.sh -c $chip -p $platform -b $board -o $os \
		-d $debug -s $sigmode -m $mode -w $programmer -v $securemode -i $tar_image -t $T -f $project_path -g $board_path
}

function createkeys()
{
	local T=$(gettop)

	if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]; then
		if [ ! -f $T/board/${RTOS_TARGET_BOARD_PATH%_*}/configs/image_header.cfg ]; then
			generate_image_header
		fi
	fi

	$T/tools/scripts/createkeys
}

function croot()
{
    T=$(gettop)
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return
    \cd $T
}

function cnetdrv()
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    \cd $(gettop)/lichee/rtos/drivers/drv/wireless/
}

function cnetthird()
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    \cd $(gettop)/lichee/rtos-components/thirdparty/network/
}

function cnetaw()
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    \cd $(gettop)/lichee/rtos-components/aw/network/service
}

function cwifimg()
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    \cd $(gettop)/lichee/rtos-components/aw/wireless/wifimanager/
}

function cboot()
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    \cd $(gettop)/lichee/brandy-2.0/u-boot-2018
}

function cboot0()
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    \cd $T/lichee/brandy-2.0/spl/
}

function cbin()
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]; then
        if [ -d "$T/board/${RTOS_TARGET_BOARD_PATH%_*}/bin"  ]; then
            \cd $T/board/${RTOS_TARGET_BOARD_PATH%_*}/bin
	    return
        fi
    fi

    if [ -d "$T/board/${RTOS_TARGET_BOARD_PATH}/bin"  ]; then
        \cd $T/board/${RTOS_TARGET_BOARD_PATH}/bin
        return
    fi

    \cd $T/board/bin
}

function crtos
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    \cd $T/lichee/rtos/
}

function cdsp
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    \cd $T/lichee/dsp/
}

function chal
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    \cd $T/lichee/rtos-hal/
}

function ccomponents
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    \cd $T/lichee/rtos-components
}

function cconfigs
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]; then
        if [ -d "$T/board/${RTOS_TARGET_BOARD_PATH%_*}/configs"  ]; then
            \cd $T/board/${RTOS_TARGET_BOARD_PATH%_*}/configs
            return
	fi
    fi

    if [ -d "$T/board/${RTOS_TARGET_BOARD_PATH}/configs"  ]; then
        \cd $T/board/${RTOS_TARGET_BOARD_PATH}/configs
        return
    fi

    \cd $T/board/configs
}

function cout()
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]; then
        if [ -d "$T/out/${RTOS_TARGET_PROJECT_PATH%_*}/"  ]; then
            \cd $T/out/${RTOS_TARGET_PROJECT_PATH%_*}/
	    return
        fi
    fi

    if [ -d "$T/out/${RTOS_TARGET_PROJECT_PATH}/"  ]; then
        \cd $T/out/${RTOS_TARGET_PROJECT_PATH}/
        return
    fi

    \cd $T/out/
}

function cbuild()
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    if [ -d "$T/lichee/rtos/build/${RTOS_PROJECT_NAME}/"  ]; then
        \cd $T/lichee/rtos/build/${RTOS_PROJECT_NAME}/
        return
    fi

    \cd $T/lichee/rtos/build/

}

function cprojects()
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    if [ -d "$T/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH}/"  ]; then
        \cd $T/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH}/
        return
    fi

    \cd $T/lichee/rtos/projects/

}

function get_arch()
{
    local T=$(gettop)
    [ -z "$T" ] && return -1

    local rtos_config=${T}/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH}/defconfig
    if grep -q "CONFIG_ARCH_ARM=y" $rtos_config; then
        echo "arm" && return
    elif grep -q "CONFIG_ARCH_RISCV=y" $rtos_config; then
        echo "riscv" && return
    elif grep -q "CONFIG_ARCH_XTENSA=y" $rtos_config; then
        echo "dsp" && return
    fi

    return -1

}

function get_chip()
{
    local T=$(gettop)
    [ -z "$T" ] && return -1

    local rtos_config=${T}/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH}/defconfig
    [ -e "${rtos_config}" ] && awk -F'[_=]' '/CONFIG_ARCH_SUN.*P.*=y/{print tolower($3)}' "${rtos_config}" | head -n 1 && return

    return -1

}

function mrtos_menuconfig()
{
    local T="$(gettop)"
    cd ${T}/lichee/rtos; make menuconfig
    cd ${T}
}

function mdsp_menuconfig()
{
	local T="$(gettop)"
	cd ${T}/lichee/dsp; ./build.sh menuconfig
	cd ${T}
}

function ota_menuconfig()
{
    local T="$(gettop)"
    cd ${T}/lichee/rtos; make ota_menuconfig
    cd ${T}
}

function mlib()
{
    echo "create lib ..."
    local T=$(gettop)

    if [ $# != 2  ] ; then
        echo "USAGE: mlib <path> <libname>"
        echo " e.g.: mlib build/r128_m33_mini/components/aw/watchpoint libwatchpoint.a"
        return 1;
    fi

    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    cd ${T}/lichee/rtos/$1

    [ $? -ne 0 ] \
        && echo "**********create lib fail***********" \
        && return 1

    ALL_OBJECTS_FILES="$(find . -type f -iname '*.o' | grep -v 'obj-in.o')"

    if [ "${RTOS_TARGET_ARCH}" == "arm"  ];then
		${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update/bin/arm-none-eabi-ar rv -o $2 ${ALL_OBJECTS_FILES}
    fi
    if [ "${RTOS_TARGET_ARCH}" == "riscv"  ];then
        ${T}/lichee/rtos/tools/riscv64-elf-x86_64-20201104/bin/riscv64-unknown-elf-ar rv -o $2 ${ALL_OBJECTS_FILES}
    fi
    [ $? -eq 0 ] && echo -e "\033[31m make ${1}/${2} successfully \033[0m"
    cd -
}

function mtfm()
{
	local T=$(gettop)

	(cd ${T}/lichee/tfm && ./build.sh)
	[ $? -ne 0 ] \
			&& echo "**********make tfm fail***********" \
			&& return 1
	(cp ${T}/lichee/tfm/out/bin/tfm_s.bin ${T}/board/${RTOS_TARGET_BOARD_PATH%_*}/bin/tfm.bin -rvf)
	(cp ${T}/lichee/tfm/out/install/interface/*  ${T}/lichee/rtos/components/aw/tfm/ -rvf)

	[ $? -ne 0 ] \
			&& echo "**********copy file fail***********" \
			&& return 1

	(echo "**********make tfm successfully***********")
}

function mdsp()
{
	local T=$(gettop)

	if [ "x$@" == "xconfig" ] ;then
		(cd ${T}/lichee/dsp && ./build.sh config)
	fi

	if [ "x$@" == "xmenuconfig" ] ;then
		(cd ${T}/lichee/dsp && ./build.sh menuconfig)
	fi

	if [ "x$@" == "x" ] ;then
		echo "build dsp ..."

		(cd ${T}/lichee/dsp && ./build.sh )
		[ $? -ne 0 ] \
			&& echo "**********make dsp fail***********" \
			&& return 1
		(install_dsp)
	fi
}

function mrtos()
{
    echo "build rtos ..."
    local T=$(gettop)

    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    [ -z "${RTOS_TARGET_PROJECT_PATH}" ] \
        && echo "Couldn't get project name. Try lunch project first." \
        && return

    [ -f ${T}/lichee/rtos/.config ] \
        && [ -z "$(diff -u ${T}/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH}/defconfig ${T}/lichee/rtos/.config)" ] || {
        echo "Use ${RTOS_TARGET_PROJECT_PATH} default config"
        cp ${T}/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH}/defconfig ${T}/lichee/rtos/.config
    }

    if [ -e ${T}/lichee/rtos/build/${RTOS_TARGET_PROJECT_PATH}/arch/common/sys_config.o ]; then
        echo "Del lichee/rtos/build/${RTOS_PROJECT_NAME}/arch/common/sys_config.o"
        rm ${T}/lichee/rtos/build/${RTOS_PROJECT_NAME}/arch/common/sys_config.o
    fi

    local JOBS=`grep -c ^processor /proc/cpuinfo`

    (cd ${T}/lichee/rtos && make -j${JOBS} $@ )
    [ $? -ne 0 ] \
        && echo "**********make rtos fail***********" \
        && return 1

    if [ "x$@" == "xmenuconfig" ]; then
            cp ${T}/lichee/rtos/.config ${T}/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH}/defconfig
            return
    fi

    local IMG_DIR=$(getimgdir)
    local BIN_DIR=${T}/$(getbindir)

    diff ${IMG_DIR}/rt_system.bin ${BIN_DIR}/freertos.fex &>/dev/null
    if [ $? -ne 0 -a -f ${IMG_DIR}/rt_system.bin ]; then
	    echo -e "\033[31mcopying ${IMG_DIR}/rt_system.bin to ${BIN_DIR}/freertos.fex\033[0m"
	    cp ${IMG_DIR}/rt_system.bin ${BIN_DIR}/freertos.fex
            echo -e "\033[32m#### make completed successfully\033[0m"
    fi

    [ -f ${IMG_DIR}/rt_system.bin ] &&
	cp -v ${IMG_DIR}/rt_system.bin ${BIN_DIR}/rtos_${RTOS_TARGET_ARCH}_${RTOS_TARGET_CHIP}.fex

    [ -f ${IMG_DIR}/rt_system_xip.bin ] &&
	cp -v ${IMG_DIR}/rt_system_xip.bin ${BIN_DIR}/rtos_xip_${RTOS_TARGET_ARCH}_${RTOS_TARGET_CHIP}.fex

    [ -f ${IMG_DIR}/rt_system_psram.bin ] &&
	cp -v ${IMG_DIR}/rt_system_psram.bin ${BIN_DIR}/rtos_psram_${RTOS_TARGET_ARCH}_${RTOS_TARGET_CHIP}.fex

    [ -f ${IMG_DIR}/rt_system_hpsram.bin ] &&
	cp -v ${IMG_DIR}/rt_system_hpsram.bin ${BIN_DIR}/rtos_hpsram_${RTOS_TARGET_ARCH}_${RTOS_TARGET_CHIP}.fex

    return 0
}
complete -W "menuconfig clean" mrtos


function ota_mrtos()
{
    echo "build rtos ota ..."
    local T=$(gettop)

    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    [ -z "${RTOS_PROJECT_NAME}" ] \
        && echo "Couldn't get project name. Try lunch project first." \
        && return

    [ -f ${T}/lichee/rtos/.config ] \
        && [ -z "$(diff -u ${T}/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH}/ota_defconfig ${T}/lichee/rtos/.config)" ] || {
        echo "Use ${RTOS_TARGET_ROJECT_PATH} default config"
        cp ${T}/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH}/ota_defconfig ${T}/lichee/rtos/.config
    }

    if [ -e ${T}/lichee/rtos/build/${RTOS_PROJECT_NAME}/arch/common/sys_config.o ]; then
        echo "Del lichee/rtos/build/${RTOS_PROJECT_NAME}/arch/common/sys_config.o"
        rm ${T}/lichee/rtos/build/${RTOS_PROJECT_NAME}/arch/common/sys_config.o
    fi

    local JOBS=`grep -c ^processor /proc/cpuinfo`

    (cd ${T}/lichee/rtos && make -j${JOBS} $@ )
    [ $? -ne 0 ] \
        && echo "**********make rtos fail***********" \
        && return 1

    if [ "x$@" == "xmenuconfig" ]; then
            cp ${T}/lichee/rtos/.config ${T}/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH}/ota_defconfig
            return
    fi

    local IMG_DIR=$(getimgdir)
    local BIN_DIR=${T}/$(getbindir)

    diff ${IMG_DIR}/rt_system.bin ${BIN_DIR}/freertos.fex &>/dev/null
    if [ $? -ne 0 -a -f ${IMG_DIR}/rt_system.bin ]; then
	    echo -e "\033[31mcopying ${IMG_DIR}/rt_system.bin to ${BIN_DIR}/freertos.fex\033[0m"
	    cp ${IMG_DIR}/rt_system.bin ${BIN_DIR}/freertos.fex
            echo -e "\033[32m#### make completed successfully\033[0m"
    fi

    [ -f ${IMG_DIR}/rt_system.bin ] &&
	cp -v ${IMG_DIR}/rt_system.bin ${BIN_DIR}/rtos_${RTOS_TARGET_ARCH}_${RTOS_TARGET_CHIP}-recovery.fex

    [ -f ${IMG_DIR}/rt_system_xip.bin ] &&
	cp -v ${IMG_DIR}/rt_system_xip.bin ${BIN_DIR}/rtos_xip_${RTOS_TARGET_ARCH}_${RTOS_TARGET_CHIP}.fex

    [ -f ${IMG_DIR}/rt_system_psram.bin ] &&
	cp -v ${IMG_DIR}/rt_system_psram.bin ${BIN_DIR}/rtos_psram_${RTOS_TARGET_ARCH}_${RTOS_TARGET_CHIP}.fex

    [ -f ${IMG_DIR}/rt_system_hpsram.bin ] &&
	cp -v ${IMG_DIR}/rt_system_hpsram.bin ${BIN_DIR}/rtos_hpsram_${RTOS_TARGET_ARCH}_${RTOS_TARGET_CHIP}.fex

    return 0
}

function print_lunch_menu()
{
    local uname=$(uname)
    echo
    echo "You're building on" $uname
    echo
    echo "Lunch menu... pick a combo:"

    local i=1
    local choice
    for choice in ${auto_complete_opts[@]}
    do
        echo "     $i. $choice"
        i=$(($i+1))
    done
    echo
}

function get_all_projects()
{
    local T=$(gettop)
    local exit_flag=0;

    if [ "${#auto_complete_opts[@]}" -ne 0 ]; then
            unset auto_complete_opts
    fi

	if [ "${#device_name_opts[@]}" -ne 0 ]; then
            unset device_name_opts
    fi

    for f in `ls -l ${T}/lichee/rtos/projects | awk '/^d/{print $NF}'`;
    do
        exit_flag=0;
        local project_name=${f};
        for n in `ls -l ${T}/lichee/rtos/projects/${f} | awk '/^d/{print $NF}'`;
        do
            project_name=${f}_${n}
            for i in ${auto_complete_opts[@]}
            do
                if [ "${i}" == "${project_name}" ]; then
                    exit_flag=1;
                    echo i=${i}
                    break;
                fi
            done
            if [ ${exit_flag} -eq 0 ]; then
                auto_complete_opts=(${auto_complete_opts[@]} ${project_name})
                device_name_opts=(${device_name_opts[@]} ${f})
            fi
        done
    done

    #search project's soft link
    for f in `ls -l ${T}/lichee/rtos/projects | awk '/^l/{print $(NF-2)}'`;
    do
        exit_flag=0;
        local project_name=${f};
        for n in `ls -l ${T}/lichee/rtos/projects/${f} | awk '/^d/{print $NF}'`;
        do
            project_name=${f}_${n}
            for i in ${auto_complete_opts[@]}
            do
                if [ "${i}" == "${project_name}" ]; then
                    exit_flag=1;
                    echo i=${i}
                    break;
                fi
            done
            if [ ${exit_flag} -eq 0 ]; then
                auto_complete_opts=(${auto_complete_opts[@]} ${project_name})
                device_name_opts=(${device_name_opts[@]} ${f})
            fi
        done
    done
}

function _lunch() {
	local cur prev

	COMPREPLY=()

	cur="${COMP_WORDS[COMP_CWORD]}"
	prev="${COMP_WORDS[COMP_CWORD-1]}"

	if [[ ${cur} == * && "${auto_complete_opts}" != *"${prev}"* ]] ; then
		COMPREPLY=( $(compgen -W "${auto_complete_opts}" -- ${cur})  )
		return 0
	fi
}

function uncompress_toolchain_linux() {
    if [ "x${RTOS_TARGET_ARCH}" == "xriscv"  ];then
        if [ ! -f "${T}/lichee/rtos/tools/riscv64-elf-x86_64-20201104/.time" ]; then
            if [ -d "${T}/lichee/rtos/tools/riscv64-elf-x86_64-20201104" ]; then
                rm -rf ${T}/lichee/rtos/tools/riscv64-elf-x86_64-20201104
            fi
            mkdir -p ${T}/lichee/rtos/tools/riscv64-elf-x86_64-20201104
            tar zxvf ${T}/lichee/rtos/tools/riscv64-elf-x86_64-20201104.tar.gz -C ${T}/lichee/rtos/tools/riscv64-elf-x86_64-20201104/
            if [ $? == 0 ]; then
                touch ${T}/lichee/rtos/tools/riscv64-elf-x86_64-20201104/.time
            fi
        fi
        export RTOS_BUILD_TOOLCHAIN=${T}/lichee/rtos/tools/riscv64-elf-x86_64-20201104/bin/riscv64-unknown-elf-
    fi
    if [ "x${RTOS_TARGET_ARCH}" == "xarm"  ];then
        if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]
        then
            if [ ! -f "${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update/.time" ]; then
                if [ -d "${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update" ]; then
                    rm -rf ${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update
                fi
                tar -jxvf ${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2 -C ${T}/lichee/rtos/tools/
                if [ $? == 0 ]; then
                    touch ${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update/.time
                fi
            fi
            export RTOS_BUILD_TOOLCHAIN=${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update/bin/arm-none-eabi-
        else
            if [ ! -f "${T}/lichee/rtos/tools/gcc-arm-melis-eabi-8-2019-q3-update/.time" ]; then
                if [ -d "${T}/lichee/rtos/tools/gcc-arm-melis-eabi-8-2019-q3-update" ]; then
                    rm -rf ${T}/lichee/rtos/tools/gcc-arm-melis-eabi-8-2019-q3-update
                fi
                tar -jxvf ${T}/lichee/rtos/tools/gcc-arm-melis-eabi-8-2019-q3-update-linux.tar.bz2 -C ${T}/lichee/rtos/tools/
                if [ $? == 0 ]; then
                    touch ${T}/lichee/rtos/tools/gcc-arm-melis-eabi-8-2019-q3-update/.time
                fi
            fi
            export RTOS_BUILD_TOOLCHAIN=${T}/lichee/rtos/tools/gcc-arm-melis-eabi-8-2019-q3-update/bin/arm-melis-eabi-
        fi
    fi

    if [ ! -f "$T/lichee/rtos/scripts/kconfig-frontends/frontends/conf/kconfig-conf" -o \
         ! -f "$T/lichee/rtos/scripts/kconfig-frontends/frontends/mconf/kconfig-mconf" ]; then
        
        cd "$T/lichee/rtos/scripts/kconfig-frontends/automake-1.15"
        ./configure --prefix="$T/lichee/rtos/scripts/kconfig-frontends/automake-1.15/1.15/"
        make
        make install
        export PATH=$T/lichee/rtos/scripts/kconfig-frontends/automake-1.15/1.15/bin:$PATH
        cd -
        
        cd "$T/lichee/rtos/scripts/kconfig-frontends"
        ./configure --enable-mconf --disable-nconf --disable-gconf --disable-qconf
        make
        cd -
    fi
}

function uncompress_toolchain_cygwin() {
    if [ "x${RTOS_TARGET_ARCH}" == "xriscv"  ];then
        if [ ! -f "${T}/lichee/rtos/tools/Xuantie-900-gcc-elf-newlib-mingw-V2.6.1/.time" ]; then
            if [ -d "${T}/lichee/rtos/tools/Xuantie-900-gcc-elf-newlib-mingw-V2.6.1" ]; then
                rm -rf ${T}/lichee/rtos/tools/Xuantie-900-gcc-elf-newlib-mingw-V2.6.1
            fi
            tar zxvf ${T}/lichee/rtos/tools/Xuantie-900-gcc-elf-newlib-mingw-V2.6.1-20220906.tar.gz -C ${T}/lichee/rtos/tools/
            if [ $? == 0 ]; then
                touch ${T}/lichee/rtos/tools/Xuantie-900-gcc-elf-newlib-mingw-V2.6.1/.time
            fi
        fi
        export RTOS_BUILD_TOOLCHAIN=${T}/lichee/rtos/tools/Xuantie-900-gcc-elf-newlib-mingw-V2.6.1/bin/riscv64-unknown-elf-
    fi
    if [ "x${RTOS_TARGET_ARCH}" == "xarm"  ];then
        if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]
        then
            if [ ! -f "${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update-win32/.time" ]; then
                if [ -d "${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update-win32" ]; then
                    rm -rf ${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update-win32
                fi
                mkdir -p ${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update-win32
                unzip ${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update-win32.zip -d ${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update-win32
                if [ $? == 0 ]; then
                    touch ${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update-win32/.time
                fi
            fi
            export RTOS_BUILD_TOOLCHAIN=${T}/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update-win32/bin/arm-none-eabi-
        fi
    fi
}

function lunch_rtos()
{
    local T="$(gettop)"
    local last
	local choice
    local i=1

    if [ -f "${T}/lichee/rtos/.config" ]; then
        last="$(sed -n -r '/CONFIG_PROJECT_[a-zA-Z_0-9]*[^_]=y/{s/CONFIG_PROJECT_([a-zA-Z_0-9]*[^_])=y/\1/;p;q}' ${T}/lichee/rtos/.config)"
        last="$(echo ${last} | sed 's/[A-Z]/\l&/g')"
    fi
    echo "last=${last}"
    # select platform
    local select
    if [ "$1" ] ; then
        select=$1
    else
        print_lunch_menu
        echo -n "Which would you like?"
        [ -n "${last}" ] && echo -n " [Default ${last}]"
        echo -n ": "
        read select
    fi

    echo "select=${select}..."
    if [ -z "${select}" ]; then
        select="${last}"
    elif (echo -n $select | grep -q -e "^[0-9][0-9]*$"); then
        if [ $select -le ${#auto_complete_opts[@]} ]; then
            select=${auto_complete_opts[$(($select-1))]}
        else
            echo "Invalid lunch combo: $select" >&2
            return 1
        fi
    fi

    for choice in ${auto_complete_opts[@]}
    do
        if [ "${select}" == "${choice}" ]; then
            export RTOS_TARGET_DEVICE=${device_name_opts[$(($i-1))]}
            break;
        fi
        i=$(($i+1))
    done

    export RTOS_TARGET_PROJECT_PATH=${RTOS_TARGET_DEVICE}/${select#${RTOS_TARGET_DEVICE}_}
    export RTOS_TARGET_BOARD_PATH=${RTOS_TARGET_DEVICE}/${select#${RTOS_TARGET_DEVICE}_}
    export RTOS_PROJECT_NAME=${select}

    if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]; then
        if [ -d "$T/board/${RTOS_TARGET_BOARD_PATH%_*}/bin"  ]; then
            export TARGET_PLATFORM=${RTOS_TARGET_BOARD_PATH%_*}
        fi
    else
        export TARGET_PLATFORM=${RTOS_PROJECT_NAME}
    fi

    #1. copy defconfig to rtos top directory.
	echo ${RTOS_TARGET_PROJECT_PATH}
    cp -v ${T}/lichee/rtos/projects/${RTOS_TARGET_PROJECT_PATH}/defconfig ${T}/lichee/rtos/.config

	export RTOS_BUILD_TOP=$(gettop)
    export RTOS_TARGET_ARCH=$(get_arch)
    export RTOS_TARGET_CHIP=$(get_chip)

    if [ "$(expr substr $(uname -s) 1 6)" == "CYGWIN" ];then
        uncompress_toolchain_cygwin
    else
        uncompress_toolchain_linux
    fi

    echo "============================================"
    echo "RTOS_BUILD_TOP=${RTOS_BUILD_TOP}"
    echo "RTOS_TARGET_ARCH=${RTOS_TARGET_ARCH}"
    echo "RTOS_TARGET_CHIP=${RTOS_TARGET_CHIP}"
    echo "RTOS_TARGET_DEVICE=${RTOS_TARGET_DEVICE}"
    echo "RTOS_PROJECT_NAME=${RTOS_PROJECT_NAME}"
    echo "============================================"
    echo -e "Run \033[32mmrtos_menuconfig\033[0m to config rtos"
    echo -e "Run \033[32mm\033[0m or \033[32mmrtos\033[0m to build rtos"
}

function lunch_nuttx()
{
    local T="${gettop}"
    ./${T}/lichee/nuttx/nuttx/tools/configure.sh -l $1
}

function mnuttx()
{
    local T="$(gettop)"

    # TODO: there is sth error with multi-thread compile
    if [ 0 -eq 1 ]; then
        local JOBS=`grep -c ^processor /proc/cpuinfo`

        (cd ${T}/lichee/nuttx/nuttx && make -j${JOBS} $@ )
        [ $? -ne 0 ] \
            && echo "**********make nuttx fail***********" \
            && return 1
    else
        cd ${T}/lichee/nuttx/nuttx/;
        make $@
    fi

    echo "copy binary to board folder ..."
    cp nuttx.bin ../../../board/bin/freertos.fex
    cd ${T}
}
function mnuttx_menuconfig()
{
    local T="$(gettop)"
    cd ${T}/lichee/nuttx/nuttx; make menuconfig
    cd ${T}
}

function cnuttx()
{
    T=$(gettop)
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return
    \cd $T/lichee/nuttx/nuttx
}

function callstack()
{
    T=$(gettop)
    local IMG_DIR=$(getimgdir)
    local toolchain
    if [ -f "${T}/lichee/rtos/.config" ]; then
	    toolchain=$(sed -n '/CONFIG_TOOLCHAIN=/p' ${T}/lichee/rtos/.config)
	    toolchain=${toolchain##*/}
	    toolchain=${toolchain%-*}
    fi
    python2 ${T}/lichee/rtos/scripts/backtrace_parser.py $1 ${IMG_DIR}/rt_system.elf -t $toolchain
}

function doaddr2line()
{
    local IMG_DIR=$(getimgdir)

    ${RTOS_BUILD_TOOLCHAIN}addr2line -e ${IMG_DIR}/rt_system.elf -a $@
}

function donm()
{
    local IMG_DIR=$(getimgdir)

    ${RTOS_BUILD_TOOLCHAIN}nm ${IMG_DIR}/rt_system.elf
}


function doobjdump()
{
    T=$(gettop)
    local IMG_DIR=$(getimgdir)

    ${RTOS_BUILD_TOOLCHAIN}objdump -d ${IMG_DIR}/rt_system.elf | less
}

function ksize()
{
    local T=$(gettop)
    local cwd=`pwd`

    \cd $T/lichee/rtos/

    if [ -f "build/${RTOS_PROJECT_NAME}/img/rt_system.map"  ]; then
		python3 scripts/ksize.py build/${RTOS_PROJECT_NAME}/img/rt_system.map build/${RTOS_PROJECT_NAME}/img/rt_system.elf
    else
		echo "Please Compile RTOS."
    fi

    \cd $cwd
}

function kmap()
{
	local T=$(gettop)
	local cwd=`pwd`

	\cd $T/lichee/rtos/

    if [ -f "build/${RTOS_PROJECT_NAME}/img/rt_system.map"  ]; then
		python2 scripts/map_parse_gcc_v3.py build/${RTOS_PROJECT_NAME}/img/rt_system.map
	else
		echo "Please Compile RTOS."
    fi

	\cd $cwd
}

function install_dsp()
{
    local T=$(gettop)
    [ -z "$T" ] \
        && echo "Couldn't locate the top of the tree.  Try setting TOP." \
        && return

    [ -z "${RTOS_PROJECT_NAME}" ] \
	&& echo -e "Error: Please run \033[31mlunch_rtos\033[0m to select project first" \
	&& return

    if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]; then
        if [ -d "$T/board/${RTOS_TARGET_BOARD_PATH%_*}/bin"  ]; then
            cp -vf $T/lichee/dsp/out/${RTOS_PROJECT_NAME%%_*}/evb1/${RTOS_PROJECT_NAME%%_*}_dsp0_evb1_raw.bin $T/board/${RTOS_TARGET_BOARD_PATH%_*}/bin/rtos_dsp_${RTOS_TARGET_CHIP}.fex
	    return
        fi
    fi

    if [ -d "$T/board/${RTOS_TARGET_BOARD_PATH}/bin"  ]; then
        cp -vf $T/lichee/dsp/out/${RTOS_PROJECT_NAME%%_*}/evb1/${RTOS_PROJECT_NAME%%_*}_dsp0_evb1_raw.bin $T/board/${RTOS_TARGET_BOARD_PATH}/bin/rtos_dsp_${RTOS_TARGET_CHIP}.fex
        return
    fi

    cp -vf $T/lichee/dsp/out/${RTOS_PROJECT_NAME%%_*}/evb1/${RTOS_PROJECT_NAME%%_*}_dsp0_evb1_raw.bin $T/board/bin/rtos_dsp_${RTOS_TARGET_CHIP}.fex
}

function print_red()
{
    echo -e '\033[0;31;1m'
    echo $1
    echo -e '\033[0m'
}

#for ota version 2, sync from tina
function ota_pack()
{
    T=$(gettop)
    \cd $T
    if [ "x${RTOS_TARGET_CHIP}" == "xsun20iw2p1" ]; then
        local BIN_DIR=$T/out/${RTOS_TARGET_PROJECT_PATH%_*}
        local CCONFIG_DIR="$T/board/${RTOS_TARGET_BOARD_PATH%_*}/configs"
	else
        local BIN_DIR=$T/out/${RTOS_PROJECT_NAME}
        local CCONFIG_DIR="$T/board/${RTOS_TARGET_BOARD_PATH}/configs"
    fi
    local OTA_DIR=$BIN_DIR/ota
    local UPDATE_CONFIG_DIR="${CCONFIG_DIR}/ota"
    local UPDATE_COMMON_CONFIG_DIR="$T/board/configs/ota"
    mkdir -p $UPDATE_CONFIG_DIR
    local CFG="ota-subimgs$1.cfg"
    mkdir -p "$OTA_DIR/update"
    local storage_type_nor=0

	if [ -e $UPDATE_CONFIG_DIR/$CFG ]; then
        local UPDATE_SUBIMGS="$UPDATE_CONFIG_DIR/$CFG"
    elif [ -e $UPDATE_COMMON_CONFIG_DIR/$CFG ]; then
        local UPDATE_SUBIMGS="$UPDATE_COMMON_CONFIG_DIR/$CFG"
    else
        print_red "can't find ota config file: $CFG"
		return 1
    fi

    unset ota_file_list
    unset ota_copy_file_list

    echo "####$UPDATE_SUBIMGS####"
    . $UPDATE_SUBIMGS

    echo ${ota_file_list[@]} | sed 's/ /\n/g'
    echo ${ota_copy_file_list[@]} | sed 's/ /\n/g'

    [ ! -f "$UPDATE_SUBIMGS" ] && print_red "$UPDATE_SUBIMGS not exist!!" &&  return 1

    echo "-------------------- config --------------------"
    echo "subimgs config by: $UPDATE_SUBIMGS"
    echo "out dir: $OTA_DIR"

    echo "-------------------- do copy --------------------"
    cp "$UPDATE_SUBIMGS" "$OTA_DIR"
    rm -f "$OTA_DIR/ota-subimgs-fix.cfg"

    # files pack into swu
    for line in ${ota_file_list[@]} ; do
        ori_file=$(echo $line | awk -F: '{print $1}')
        base_name=$(basename "$line")
        fix_name=${base_name#*:}
        [ ! -f "$ori_file" ] && print_red "$ori_file not exist!!" && return 1
        cp $ori_file $OTA_DIR/update/$fix_name
        echo $fix_name >> "$OTA_DIR/ota-subimgs-fix.cfg"
    done

    \cd - > /dev/null
}


[ -e ./tools/scripts/.hooks/expand_func ] &&
    source ./tools/scripts/.hooks/expand_func

envsetup

if [ "$(expr substr $(uname -s) 1 6)" == "CYGWIN" ];then
    echo "=============== cygwin env ==================="
    if [ -L $(gettop)/lichee/rtos/drivers/rtos-hal ];then
        echo "rm $(gettop)/lichee/rtos/drivers/rtos-hal -rf"
        rm $(gettop)/lichee/rtos/drivers/rtos-hal -rf
    fi
    if [ ! -f $(gettop)/lichee/rtos/drivers/rtos-hal/.time ];then
        echo "Copy $(gettop)/lichee/rtos-hal/ to $(gettop)/lichee/rtos/drivers/"
        cp $(gettop)/lichee/rtos-hal $(gettop)/lichee/rtos/drivers/ -rf
        touch $(gettop)/lichee/rtos/drivers/rtos-hal/.time
    fi

    if [ -L $(gettop)/lichee/rtos/components/common ];then
        rm $(gettop)/lichee/rtos/components/common -rf
    fi
    if [ ! -f $(gettop)/lichee/rtos/components/common/.time ];then
        echo "Copy $(gettop)/lichee/rtos-components/ to $(gettop)/lichee/rtos/components/common"
        cp $(gettop)/lichee/rtos-components/ $(gettop)/lichee/rtos/components/common -rf
        touch $(gettop)/lichee/rtos/components/common/.time
    fi
    echo "=============== copy done ==================="
fi

export CROSSDEV=arm-melis-eabi-
export ARCROSSDEV=arm-melis-eabi-
export PATH=$PATH:$(gettop)/lichee/rtos/tools/gcc-arm-melis-eabi-8-2019-q3-update/bin/
export PATH=$PATH:$(gettop)/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update/bin/
export PATH=$PATH:$(gettop)/lichee/rtos/tools/gcc-arm-none-eabi-8-2019-q3-update-win32/bin/
export PATH=$PATH:$(gettop)/lichee/rtos/tools/riscv64-elf-x86_64-20201104/bin/
export PATH=$PATH:$(gettop)/lichee/rtos/tools/Xuantie-900-gcc-elf-newlib-mingw-V2.6.1/bin/
