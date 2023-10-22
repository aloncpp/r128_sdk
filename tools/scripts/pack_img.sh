#!/bin/bash
#
# pack/pack
# (c) Copyright 2013 - 2016 Allwinner
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# James Deng <csjamesdeng@allwinnertech.com>
# Trace Wong <wangyaliang@allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

############################ Notice #####################################
# a. Some config files priority is as follows:
#    - xxx_${platform}.{cfg|fex} > xxx.{cfg|fex}
#    - ${chip}/${board}/*.{cfg|fex} > ${chip}/default/*.{cfg|fex}
#    - ${chip}/default/*.cfg > common/imagecfg/*.cfg
#    - ${chip}/default/*.fex > common/partition/*.fex

enable_pause=0

function get_char()
{
	SAVEDSTTY=`stty -g`
	stty -echo
	stty cbreak
	dd if=/dev/tty bs=1 count=1 2> /dev/null
	stty -raw
	stty echo
	stty $SAVEDSTTY
}

function pause()
{
	if [ "x$1" != "x" ] ;then
		echo $1
	fi
	if [ $enable_pause -eq 1 ] ; then
		echo "Press any key to continue!"
		char=`get_char`
	fi
}

function pack_error()
{
	echo -e "\033[47;31mERROR: $*\033[0m"
}

function pack_warn()
{
	echo -e "\033[47;34mWARN: $*\033[0m"
}

function pack_info()
{
	echo -e "\033[47;30mINFO: $*\033[0m"
}

source tools/scripts/shflags

# define option, format:
#   'long option' 'default value' 'help message' 'short option'
DEFINE_string 'chip' '' 'chip to build, e.g. sun7i' 'c'
DEFINE_string 'platform' '' 'platform to build, e.g. linux, android, camdroid' 'p'
DEFINE_string 'board' '' 'board to build, e.g. evb' 'b'
DEFINE_string 'os' '' 'os to build e.g. freertos' 'o'
DEFINE_string 'debug_mode' 'uart0' 'config debug mode, e.g. uart0, card0' 'd'
DEFINE_string 'signture' 'none' 'pack boot signture to do secure boot' 's'
DEFINE_string 'secure' 'none' 'pack secure boot with -v arg' 'v'
DEFINE_string 'mode' 'normal' 'pack dump firmware' 'm'
DEFINE_string 'topdir' 'none' 'sdk top dir' 't'
DEFINE_string 'programmer' '' 'creat programmer img or not' 'w'
DEFINE_string 'tar_image' '' 'creat downloadfile img .tar.gz or not' 'i'
DEFINE_string 'project_path' '' 'project path' 'f'
DEFINE_string 'board_path' '' 'board path' 'g'

# parse the command-line
FLAGS "$@" || exit $?
eval set -- "${FLAGS_ARGV}"

PACK_CHIP=${FLAGS_chip}
PACK_PLATFORM=${FLAGS_platform}
PACK_PROJECT_PATH=${FLAGS_project_path}
PACK_BOARD_PATH=${FLAGS_board_path}
PACK_BOARD=${FLAGS_board}
PACK_DEBUG=${FLAGS_debug_mode}
PACK_SIG=${FLAGS_signture}
PACK_SECURE=${FLAGS_secure}
PACK_MODE=${FLAGS_mode}
PACK_PROGRAMMER=${FLAGS_programmer}
PACK_TAR_IMAGE=${FLAGS_tar_image}
PACK_TOPDIR=${FLAGS_topdir}
PACK_OS="${FLAGS_os}"

if [ "$(expr substr $(uname -s) 1 6)" == "CYGWIN" ];then
PACK_HOSTOS="win"
else
PACK_HOSTOS="linux"
fi

MULTI_CONFIG_INDEX=0

fs_align_size=256

# the size for user space is 114688 KB for nand
# the mbr size is 256K default
LOGICAL_PARTS_KB_FOR_128Mnand=$(( 114688 - 256 ))
LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_128Mnand=0
LOGICAL_PARTS_KB_FOR_256Mnand=$(( 234240 - 256  ))
LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_256Mnand=0
LOGICAL_PARTS_KB_FOR_512Mnand=$(( 458752 - 256  ))
LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_512Mnand=0

LOGICAL_PARTS_KB_FOR_64M=$(( 64 * 1024 - 128))
LOGICAL_PARTS_KB_FOR_32M=$(( 32 * 1024 - 128 - 16))
LOGICAL_PARTS_KB_FOR_16M=$(( 16 * 1024 - 128 - 16))
LOGICAL_PARTS_KB_FOR_8M=$(( 8 * 1024 - 128 - 16))
LOGICAL_PARTS_KB_FOR_4M=$(( 4 * 1024 - 128 - 16))
LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_64M=0
LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_32M=0
LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_16M=0
LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_8M=0
LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_4M=0
current_rtos_full_img_size=0

SUFFIX=""
storage_type=""

ROOT_DIR=${PACK_TOPDIR}/out
if [ "x${PACK_HOSTOS}" == "xlinux" ] ; then
export PATH=${PACK_TOPDIR}/tools/tool:$PATH
elif [ "x${PACK_HOSTOS}" == "xwin" ] ; then
export PATH=${PACK_TOPDIR}/tools/win-tools:$PATH
fi

tools_file_list=(
tools/image-file/*.fex
)

configs_file_list=(
board/common/configs/*.fex
board/common/configs/*.cfg
board/common/configs/*.ini
board/common/sign_config/*.cfg
board/common/sign_config/*.cnf
board/common/version/version_base.mk
board/${PACK_PROJECT_PATH}/configs/*.fex
board/${PACK_PROJECT_PATH}/configs/*.cfg
board/${PACK_PROJECT_PATH}/sign_config/*.cfg
board/${PACK_PROJECT_PATH}/sign_config/*.cnf
board/${PACK_PROJECT_PATH}/version/version.mk
board/${PACK_PROJECT_PATH}/version/version_base.mk
)

boot_file_list=(
board/common/bin/boot0_nand_${PACK_CHIP}.bin:image/boot0_nand.fex
board/common/bin/boot0_sdcard_${PACK_CHIP}.bin:image/boot0_sdcard.fex
board/common/bin/boot0_spinor_${PACK_CHIP}.bin:image/boot0_spinor.fex
board/common/bin/boot0_spinor_${PACK_CHIP}_psram.bin:image/boot0_spinor_psram.fex
board/common/bin/boot0_spinor_${PACK_CHIP}_hpsram.bin:image/boot0_spinor_hpsram.fex
board/common/bin/boot0_spinor_${PACK_CHIP}_hpsram_psram.bin:image/boot0_spinor_hpsram_psram.fex
board/common/bin/fes1_${PACK_CHIP}.bin:image/fes1.fex
board/common/bin/u-boot-${PACK_CHIP}.bin:image/u-boot.fex
board/common/bin/freertos.fex:image/freertos.fex
board/common/bin/rtos_arm_${PACK_CHIP}.fex:image/rtos_arm.fex
board/common/bin/rtos_xip_arm_${PACK_CHIP}.fex:image/rtos_xip_arm.fex
board/common/bin/rtos_psram_arm_${PACK_CHIP}.fex:image/psram_arm.fex
board/common/bin/rtos_hpsram_arm_${PACK_CHIP}.fex:image/rtos_hpsram_arm.fex
board/common/bin/rtos_riscv_${PACK_CHIP}.fex:image/rtos_riscv.fex
board/common/bin/rtos_xip_riscv_${PACK_CHIP}.fex:image/rtos_xip_rv.fex
board/common/bin/rtos_psram_riscv_${PACK_CHIP}.fex:image/rtos_psram_riscv.fex
board/common/bin/rtos_hpsram_riscv_${PACK_CHIP}.fex:image/rtos_hpsram_riscv.fex
board/common/bin/rtos_dsp_${PACK_CHIP}.fex:image/rtos_dsp.fex
board/common/bin/rtos_xip_dsp_${PACK_CHIP}.fex:image/rtos_xip_dsp.fex
board/common/bin/rtos_psram_dsp_${PACK_CHIP}.fex:image/rtos_psram_dsp.fex
board/common/bin/rtos_hpsram_dsp_${PACK_CHIP}.fex:image/rtos_hpsram_dsp.fex
lichee/rtos/build/img/${PACK_PROJECT_PATH}/rt_system.bin:image/rt_system.bin
lichee/rtos/build/img/${PACK_PROJECT_PATH}/rt_system_xip.bin:image/rt_system_xip.bin
lichee/rtos/build/img/${PACK_PROJECT_PATH}/rt_system_psram.bin:image/rt_system_psram.bin
lichee/rtos/build/img/${PACK_PROJECT_PATH}/rt_system_hpsram.bin:image/rt_system_hpsram.bin
lichee/rtos/build/${PACK_PROJECT_PATH}/img/rt_system.bin:image/rt_system.bin
lichee/rtos/build/${PACK_PROJECT_PATH}/img/rt_system_xip.bin:image/rt_system_xip.bin
lichee/rtos/build/${PACK_PROJECT_PATH}/img/rt_system_psram.bin:image/rt_system_psram.bin
lichee/rtos/build/${PACK_PROJECT_PATH}/img/rt_system_hpsram.bin:image/rt_system_hpsram.bin
board/${PACK_PROJECT_PATH}/bin/boot0_sdcard_${PACK_CHIP}.bin:image/boot0_sdcard.fex
board/${PACK_PROJECT_PATH}/bin/boot0_nand_${PACK_CHIP}.bin:image/boot0_nand.fex
board/${PACK_PROJECT_PATH}/bin/boot0_spinor_${PACK_CHIP}.bin:image/boot0_spinor.fex
board/${PACK_PROJECT_PATH}/bin/boot0_spinor_${PACK_CHIP}_psram.bin:image/boot0_spinor_psram.fex
board/${PACK_PROJECT_PATH}/bin/boot0_spinor_${PACK_CHIP}_hpsram.bin:image/boot0_spinor_hpsram.fex
board/${PACK_PROJECT_PATH}/bin/boot0_spinor_${PACK_CHIP}_hpsram_psram.bin:image/boot0_spinor_hpsram_psram.fex
board/${PACK_PROJECT_PATH}/bin/fes1_${PACK_CHIP}.bin:image/fes1.fex
board/${PACK_PROJECT_PATH}/bin/u-boot-${PACK_CHIP}.bin:image/u-boot.fex
board/${PACK_PROJECT_PATH}/bin/freertos.fex:image/freertos.fex
board/${PACK_PROJECT_PATH}/bin/rtos_arm_${PACK_CHIP}.fex:image/rtos_arm.fex
board/${PACK_PROJECT_PATH}/bin/rtos_arm_${PACK_CHIP}-recovery.fex:image/recovery.fex
board/${PACK_PROJECT_PATH}/bin/etf_${PACK_CHIP}.fex:image/etf.fex
board/${PACK_PROJECT_PATH}/bin/rtos_xip_arm_${PACK_CHIP}.fex:image/rtos_xip_arm.fex
board/${PACK_PROJECT_PATH}/bin/rtos_psram_arm_${PACK_CHIP}.fex:image/psram_arm.fex
board/${PACK_PROJECT_PATH}/bin/rtos_hpsram_arm_${PACK_CHIP}.fex:image/rtos_hpsram_arm.fex
board/${PACK_PROJECT_PATH}/bin/rtos_riscv_${PACK_CHIP}.fex:image/rtos_riscv.fex
board/${PACK_PROJECT_PATH}/bin/rtos_xip_riscv_${PACK_CHIP}.fex:image/rtos_xip_rv.fex
board/${PACK_PROJECT_PATH}/bin/rtos_psram_riscv_${PACK_CHIP}.fex:image/rtos_psram_riscv.fex
board/${PACK_PROJECT_PATH}/bin/rtos_hpsram_riscv_${PACK_CHIP}.fex:image/rtos_hpsram_riscv.fex
board/${PACK_PROJECT_PATH}/bin/rtos_dsp_${PACK_CHIP}.fex:image/rtos_dsp.fex
board/${PACK_PROJECT_PATH}/bin/rtos_xip_dsp_${PACK_CHIP}.fex:image/rtos_xip_dsp.fex
board/${PACK_PROJECT_PATH}/bin/rtos_psram_dsp_${PACK_CHIP}.fex:image/rtos_psram_dsp.fex
board/${PACK_PROJECT_PATH}/bin/rtos_hpsram_dsp_${PACK_CHIP}.fex:image/rtos_hpsram_dsp.fex
board/common/bin/tfm.bin:image/tfm.fex
board/${PACK_PROJECT_PATH}/bin/tfm.bin:image/tfm.fex
board/${PACK_PROJECT_PATH}/bin/boot0_offline_secure_${PACK_CHIP}.bin:image/boot0_offline_secure_${PACK_CHIP}.fex
)

boot_file_secure=(
board/common/bin/sboot_${PACK_CHIP}.bin:image/sboot.bin
board/common/bin/tfm.bin:image/tfm.fex
board/common/bin/tfm_ns.bin:image/tfm_ns.bin
board/${PACK_PROJECT_PATH}/bin/sboot_${PACK_CHIP}.bin:image/sboot.bin
board/${PACK_PROJECT_PATH}/bin/tfm.bin:image/tfm.fex
board/${PACK_PROJECT_PATH}/bin/tfm_ns.bin:image/tfm_ns.bin
board/${PACK_PROJECT_PATH}/configs/sys_partition_nor_sec.patch:image/sys_partition_nor_sec.patch
)


if [ "x${PACK_HOSTOS}" == "xlinux" ] ; then
CUT_TRAILING_BYTES=cut-trailing-bytes
DRAGON=dragon
DRAGONSECBOOT=dragonsecboot
EXTRACTLFS=extractlfs
GENRSA=genrsa
LZ4=lz4
MBR_CONVERT_TO_GPT=mbr_convert_to_gpt
MERGE_FULL_RTOS_IMG=merge_full_rtos_img
MKENVIMAGE=mkenvimage
MKIMAGE_HEADER=mkimage_header
MKLFS=mklfs
PARSER_MBR=parser_mbr
REQ=req
RSA=rsa
SCRIPT=script
UPDATE_BOOT0=update_boot0
UPDATE_FES1=update_fes1
UPDATE_MBR=update_mbr
UPDATE_RTOS=update_rtos
UPDATE_RTOS_SBOOT=update_rtos_sboot
UPDATE_TOC0=update_toc0
UPDATE_TOC1=update_toc1
UPDATE_UBOOT=update_uboot
elif [ "x${PACK_HOSTOS}" == "xwin" ] ; then
CUT_TRAILING_BYTES=cut-trailing-bytes.exe
DRAGON=dragon.exe
DRAGONSECBOOT=dragonsecboot.exe
EXTRACTLFS=
GENRSA=
LZ4=lz4.exe
MBR_CONVERT_TO_GPT=convert_gpt.exe
MERGE_FULL_RTOS_IMG=merge_package_rtos.exe
MKENVIMAGE=mkenvimage.exe
MKIMAGE_HEADER=mkimage_header.exe
MKLFS=mklfs.exe
PARSER_MBR=parse_mbr.exe
REQ=
RSA=
SCRIPT=mode_script.exe
UPDATE_BOOT0=update_boot0.exe
UPDATE_FES1=update_fes1.exe
UPDATE_MBR=update_mbr.exe
UPDATE_RTOS=update_rtos.exe
UPDATE_RTOS_SBOOT=update_rtos_sboot.exe
UPDATE_TOC0=update_toc0.exe
UPDATE_TOC1=update_toc1.exe
UPDATE_UBOOT=update_uboot.exe
fi

# get_part_info <totol_KB_for_all_partitions> <path_of_sys_partition>
function get_part_info()
{
	sed 's/\r//g' $2 | awk -v total=$1 '
	BEGIN {
		i = 0
	}
	/^\[partition\]/ {
		info["name"] = "None"
		info["size"] = 0
		info["downloadfile"] = "\"None\""

		while (getline && $0 ~ /=/)
			info[$1] = $3
		info["size"] /= 2
		if (info["name"] == "UDISK") {
			if (info["size"] == 0) {
				info["size"] = total - sum
			}
		} else {
			sum += info["size"]
		}
		info[i] = info["name"] ":" info["size"] ":" info["downloadfile"]
		i++
	};
	END {
		for (j = 0; j < i; j++)
			print info[j]
	}'
}

function get_partition_downfile_size()
{
	local downloadfile_name=`echo $1 | awk -F '=' '{print $2}'`
	if [ ! -f ${downloadfile_name} ]; then
		echo "  file ${downloadfile_name} not find"
	else
		if [ -L ${downloadfile_name} ]; then
			local downloadfile_name_link=`readlink -f ${downloadfile_name}`
			local linkfile_name=${downloadfile_name_link##*/}
			echo "  ${downloadfile_name} -> ${downloadfile_name_link}"
			if [ ! -f ${downloadfile_name_link} ]; then
				echo "  link file ${linkfile_name} not find"
			else
				local linkfile_size=`ls -lh ${downloadfile_name_link} | awk '{print $5}'`
				echo "  ${linkfile_name} size : ${linkfile_size} byte"
			fi
		else
			local downloadfile_size=`ls -lh ${downloadfile_name} | awk '{print $5}'`
			echo "  ${downloadfile_name} size : ${downloadfile_size} byte"
		fi
	fi
}

function get_partition_mbr_size()
{
	local partition_size_name=`echo $1 | awk -F '=' '{print $1}' | sed 's/partition/mbr/g'`
	local partition_size=`echo $1 | awk -F '=' '{print $2}'`
	echo "  ${partition_size_name}  : ${partition_size} Kbyte"
}

function show_partition_message()
{
	grep -c '[mbr]' $1 > /dev/null
	if [ $? -eq 0 ]; then
		cp $1 ./show_sys_partition.tmp;
		sed -i '/^[\r;]/d' ./show_sys_partition.tmp;
		sed -i '/partition_start/d' ./show_sys_partition.tmp;
		sed -i '/user_type/d' ./show_sys_partition.tmp;
		sed -i 's/\[partition\]/------------------------------------/g' ./show_sys_partition.tmp;
		sed -i 's/[ "\r]//g' ./show_sys_partition.tmp;
		sed -i '/^[;]/d' ./show_sys_partition.tmp;
		sed -i 's/name/partition_name/g' ./show_sys_partition.tmp;
		sed -i 's/size/partition_size/g' ./show_sys_partition.tmp;
		echo ------------------------------------
		while read line
		do
			if [ "$line" == "------------------------------------" ];then
				echo $line
			else
				echo "  $line" | sed 's/=/  : /g'
				echo "  $line" | grep "mbr" >> /dev/null
				if [ $? -eq 0 ]; then
					read line
					get_partition_mbr_size $line
				fi
				echo $line | grep "downloadfile" >> /dev/null
				if [ $? -eq 0 ]; then
					get_partition_downfile_size $line
				fi
			fi
		done < ./show_sys_partition.tmp
		echo ------------------------------------
		rm ./show_sys_partition.tmp
	else
		echo "==========input is not a partition file=========="
	fi
}

#mkspiffs <size_in_bytes> <input_directory> <output_file>
function mkspiffs()
{
	${PACK_TOPDIR}/tools/scripts/spiffsgen.py --meta-len 0 $1 $2 $3
}

function mklittlefs()
{
	echo "--- creating littlefs image ---"

	if [ "x${PACK_HOSTOS}" == "xlinux" ] ; then
		${MKLFS} -c $2 -b $((4 * 1024)) -r 256 -p 256 -s $1 -i $3
		if [ $? -ne 0 ]; then
			pack_error "create littlefs failed"
			exit 1
		fi
	elif [ "x${PACK_HOSTOS}" == "xwin" ] ; then
		cp ${PACK_TOPDIR}/tools/win-tools/msvcr120.dll ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
		local src=$(cygpath -w $2)
		local dst=$(cygpath -w $3)
		${MKLFS} -c ${src/\\/\\\\} -b $((4 * 1024)) -r 256 -p 256 -s $1 -i ${dst/\\/\\\\}
		echo "${MKLFS} -c ${src/\\/\\\\} -b $((4 * 1024)) -r 256 -p 256 -s $1 -i ${dst/\\/\\\\}"
	fi

	echo "--- create littlefs image end ---"
}

function make_rtos_reserve_image()
{
	local part name size downloadfile imageSize reserveSize

	for part in $(get_part_info ${2} ${1})
	do
		name="$(awk -F: '{print $1}' <<< "${part}")"
		size="$(awk -F: '{print $2}' <<< "${part}")"
		downloadfile="$(awk -F: '{print $3}' <<< "${part}" | sed 's/"//g')"

		if [ -z ${downloadfile} ] ; then
			continue
		fi

		if [ "x${downloadfile}" == "xNone" ] ; then
			continue
		fi

		if [ "x${name}" == "x${3}" ] ; then
			part_name=${name}
			part_size=$(( ${size} * 1024 / 512 ))
		else
			continue
		fi
		size=$(( ${size} * 1024 ))
		size=$(( ${size} & (~(4 * 1024 - 1)) ))
		echo "try to create image: ${name} to ${downloadfile} with size ${size}"

		if [ -L ${downloadfile} ]; then
			local downloadfile_link=`readlink -f ${downloadfile}`
			local linkfile_name=${downloadfile_link##*/}
			fileSize=`ls -l ${downloadfile_link} | awk '{print $5}'`
		else
			fileSize=`ls -l ${downloadfile} | awk '{print $5}'`
		fi
		imageSize=$(( ${fileSize} ))
		imageSize=$(( ${imageSize} + (4 * 1024 -1) ))
		imageSize=$(( ${imageSize} & (~(4 * 1024 - 1)) ))

		reserveSize=$(( ${size} - ${imageSize} ))
		reserveSize=$(( ${reserveSize} & (~(4 * 1024 - 1)) ))

		dir_name="${PACK_TOPDIR}/board/${PACK_PROJECT_PATH}/data/${4}"
		if [ ! -d ${dir_name} ]; then
			pack_error "not found ${dir_name} for creating ${downloadfile} ${imageSize} ${reserveSize} ${appendSize}, try default data!!!!!"
			dir_name="${PACK_TOPDIR}/board/common/data/${4}"
			if [ ! -d ${dir_name} ]; then
				pack_error "not found ${dir_name} for creating ${downloadfile} ${imageSize} ${reserveSize} ${appendSize}"
				continue
			fi
		fi
		name="${dir_name}"
		echo "The filesystem path: ${name}"

		appendSize=$(( ${imageSize} - ${fileSize} ))
		mklittlefs ${reserveSize} ${name} ${4}_reserve_image.fex
		dd if=/dev/zero of=${4}_append.fex bs=1 count=${appendSize}
		cat ${downloadfile} ${4}_append.fex ${4}_reserve_image.fex > ${4}.fex
		rm ${downloadfile}
		rm rtos_pkg.fex
		mv ${4}.fex rtos_toc1.fex
		ln -s rtos_toc1.fex rtos_pkg.fex
		ln -s rtos_toc1.fex ${downloadfile}

		rm ${4}_reserve_image.fex
		if [ -f ${4}_append.fex ] ; then
			rm ${4}_append.fex
		fi
	done
}

function make_pstore_image()
{
	local part name size downloadfile

	for part in $(get_part_info ${2} ${1})
	do
		name="$(awk -F: '{print $1}' <<< "${part}")"
		size="$(awk -F: '{print $2}' <<< "${part}")"
		downloadfile="$(awk -F: '{print $3}' <<< "${part}" | sed 's/"//g')"

		if [ "x${name}" == "xpstore" ] ; then
			part_name=${name}
			part_size=$(( ${size} * 1024 / 512  ))
			eval ${3}=${part_size}

			grep "^pstore.fex" <<< "${downloadfile}" || continue
			size=$(( ${size} * 1024  ))

			echo "try to create image: ${name} to ${downloadfile} with size ${size}"

			tr '\000' '\377' < /dev/zero | dd of=${downloadfile} bs=1 count=${size}
		fi
	done
}

function make_data_image()
{
	local part name size downloadfile dir_name

	for part in $(get_part_info ${2} ${1})
	do
		name="$(awk -F: '{print $1}' <<< "${part}")"
		size="$(awk -F: '{print $2}' <<< "${part}")"
		downloadfile="$(awk -F: '{print $3}' <<< "${part}" | sed 's/"//g')"
		grep "^data" <<< "${downloadfile}" || continue

		if [ "x${name}" == "xUDISK" ] ; then
			part_name=${name}
			part_size=$(( ${size} * 1024 / 512 ))
		fi
		size=$(( ${size} * 1024 ))
		size=$(( ${size} & (~(128 * 1024 - 1)) ))
		echo "try to create image: ${name} to ${downloadfile} with size ${size}"
		dir_name="${PACK_TOPDIR}/board/${PACK_PROJECT_PATH}/data/${name}"
		if [ ! -d ${dir_name} ]; then
			pack_error "not found ${dir_name} for creating ${downloadfile}, try default data!!!!!"
			dir_name="${PACK_TOPDIR}/board/common/data/${name}"
			if [ ! -d ${dir_name} ]; then
				pack_error "not found ${dir_name} for creating ${downloadfile}"
				continue
			fi
		fi
		name="${dir_name}"
		echo "The filesystem path: ${name}"

		downloadfile="${ROOT_DIR}/${PACK_PROJECT_PATH}/image/${downloadfile}"
		#mkspiffs ${size} ${name} ${downloadfile}
		mklittlefs ${size} ${name} ${downloadfile}
		#if have UDISK file,update UDISK size info to partition config file
		if [ $? -eq 0 ] ; then
			if [ "x${part_name}" == "xUDISK" ] ; then
				eval ${3}=${part_size}
			fi
		fi

		if [ x"${4}" == x"cut_zero" ]; then
			echo "cut zero to reduce fs size"
			mv ${downloadfile} ${downloadfile}.ori
			cp ${downloadfile}.ori ${downloadfile}.cut
			if [ "x${PACK_HOSTOS}" == "xlinux" ] ; then
				${CUT_TRAILING_BYTES} ${downloadfile}.cut
			elif [ "x${PACK_HOSTOS}" == "xwin" ] ; then
				local cut_file=$(cygpath -w ${downloadfile}.cut)
				${CUT_TRAILING_BYTES} ${cut_file/\\/\\\\}
			fi
			dd if=${downloadfile}.cut of=${downloadfile} bs=$fs_align_size conv=sync
		fi
	done
}

function make_spiffs_image()
{
	local part name size downloadfile dir_name

	for part in $(get_part_info ${2} ${1})
	do
		name="$(awk -F: '{print $1}' <<< "${part}")"
		size="$(awk -F: '{print $2}' <<< "${part}")"
		downloadfile="$(awk -F: '{print $3}' <<< "${part}" | sed 's/"//g')"
		grep "^spiffs" <<< "${downloadfile}" || continue

		if [ "x${name}" == "xUDISK" ] ; then
			part_name=${name}
			part_size=$(( ${size} * 1024 / 512 ))
		fi
		size=$(( ${size} * 1024 ))
		echo "try to create spiffs image: ${name} to ${downloadfile} with size ${size}"
		dir_name="${PACK_TOPDIR}/board/${PACK_PROJECT_PATH}/data/${name}"
		if [ ! -d ${dir_name} ]; then
			pack_error "not found ${dir_name} for creating ${downloadfile}, try default data!!!!!"
			dir_name="${PACK_TOPDIR}/board/common/data/${name}"
			if [ ! -d ${dir_name} ]; then
				pack_error "not found ${dir_name} for creating ${downloadfile}"
				continue
			fi
		fi
		name="${dir_name}"
		echo "The filesystem path: ${name}"

		downloadfile="${ROOT_DIR}/${PACK_PROJECT_PATH}/image/${downloadfile}"
		mkspiffs ${size} ${name} ${downloadfile}
		#if have UDISK file,update UDISK size info to partition config file
		if [ $? -eq 0 ] ; then
			if [ "x${part_name}" == "xUDISK" ] ; then
				eval ${3}=${part_size}
			fi
		fi
	done
}

function do_prepare()
{
	rm -rf ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
	mkdir -p ${ROOT_DIR}/${PACK_PROJECT_PATH}/image

	printf "copying tools file\n"
	for file in ${tools_file_list[@]} ; do
		cp -f $file ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/ 2> /dev/null
	done

	printf "copying configs file\n"
	for file in ${configs_file_list[@]} ; do
		cp -f $file ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/ 2> /dev/null
	done

	if [ "x${PACK_MODE}" = "xdump" ] ; then
		cp -vf ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_partition_dump.fex ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_partition.fex
		cp -vf ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_partition_nor_dump.fex ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_partition_nor.fex
		cp -vf ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/usbtool_test.fex ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/usbtool.fex
	fi

	local config_xip=0;
	if [ -f "${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_header.cfg" ]; then
		grep -q "xip" ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_header.cfg
		if [ $? -eq 0 ]; then
			config_xip=1
		fi
	fi
	storage_type=`sed -e '/^$/d' -e '/^;/d' -e '/^\[/d' -n -e '/^storage_type/p' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_config.fex | sed 's/=/ /g' | awk '{ print $2;}'`
	echo "storage_type value is ${storage_type}"
	image_instruction="image is for nand/emmc"

	case ${storage_type} in
		3)
		echo "storage type is nor"
		image_instruction="image is for nor"
		IMG_FLASH="_nor"
		if [ -f ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_nor.cfg ];then
			echo "image_nor.cfg is exist"
			mv ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_nor.cfg ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg && echo "mv image_nor.cfg image.cfg"
		fi
		if [ -f ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/env_nor.cfg ];then
			echo "env_nor.cfg is exist"
			mv ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/env_nor.cfg ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/env.cfg && echo "mv env_nor.cfg env.cfg"
		fi
		;;
		-1)
		;;
		*)
		if [ "x${storage_type}" = "x5" ] ; then
			IMG_FLASH="_nand"
		fi
		if [ -f ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_partition_nor.fex ];then
			rm ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_partition_nor.fex && echo "rm ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_partition_nor.fex"
		fi
		if [ -f ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_nor.cfg ];then
			rm ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_nor.cfg && echo "rm ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_nor.cfg"
		fi
		if [ -f ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/env_nor.cfg ];then
			rm ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/env_nor.cfg && echo "rm ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/env_nor.cfg"
		fi
		;;
	esac

	for file in ${boot_file_list[@]} ; do
		cp -f `echo $file | awk -F: '{print $1}'` \
			${ROOT_DIR}/${PACK_PROJECT_PATH}/`echo $file | awk -F: '{print $2}'` 2> /dev/null
	done

	[ -f ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/freertos.fex ] && {
		gzip -c ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/freertos.fex > ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/freertos-gz.fex
		if [ "x${PACK_HOSTOS}" == "xlinux" ] ; then
			${LZ4} ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/freertos.fex ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/freertos-lz4.fex
		elif [ "x${PACK_HOSTOS}" == "xwin" ] ; then
			local freertos_fex=$(cygpath -w ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/freertos.fex)
			local freertos_lz4_fex=$(cygpath -w ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/freertos-lz4.fex)
			${LZ4} ${freertos_fex/\\/\\\\} ${freertos_lz4_fex/\\/\\\\}
		fi
	}

	if [ "x${PACK_SECURE}" = "xsecure" -o "x${PACK_SIG}" = "xsecure" -o  "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		printf "copying secure boot file\n"
		for file in ${boot_file_secure[@]} ; do
			cp -f `echo $file | awk -F: '{print $1}'` \
				${ROOT_DIR}/${PACK_PROJECT_PATH}/`echo $file | awk -F: '{print $2}'` 2>/dev/null
		done
	fi

	if [ "x${config_xip}" = "x1" ] ; then
		if [ "x${PACK_SECURE}" = "xsecure"  -o "x${PACK_SIG}" = "xsecure" ] ; then
			echo "CONFIG_XIP=y, use sys_partition_xip.fex"
			cp -vf ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_partition_xip.fex ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_partition_nor.fex
		else
			echo "CONFIG_XIP=y, use sys_partition_xip.fex"
			cp -vf ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_partition_xip.fex ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_partition_nor.fex
		fi
	fi

	if [ "x${PACK_SECURE}" = "xsecure"  -o "x${PACK_SIG}" = "xsecure" ] ; then
		printf "add burn_secure_mode in target in sys config\n"
		sed -i -e '/^\[target\]/a\burn_secure_mode=1' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_config${SUFFIX}.fex
		sed -i -e '/^\[platform\]/a\secure_without_OS=0' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_config${SUFFIX}.fex
	elif [ "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		printf "add burn_secure_mode in target in sys config\n"
		sed -i -e '/^\[target\]/a\burn_secure_mode=1' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_config${SUFFIX}.fex
		sed -i -e '/^\[platform\]/a\secure_without_OS=1' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_config${SUFFIX}.fex
	else
		sed -i '/^burn_secure_mod/d' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_config${SUFFIX}.fex
		sed -i '/^secure_without_OS/d' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/sys_config${SUFFIX}.fex
	fi

	sed -i 's/\\\\/\//g' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg
	sed -i 's/^imagename/;imagename/g' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg

	IMG_NAME="${PACK_PLATFORM}_${PACK_OS}_${PACK_BOARD}_${PACK_DEBUG}"

	if [ "x${PACK_SIG}" != "xnone" ]; then
		IMG_NAME="${IMG_NAME}_${PACK_SIG}"
	fi

	if [ "x${PACK_MODE}" = "xdump" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_MODE}"
	fi

	if [ "x${PACK_SECURE}" = "xsecure" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_SECURE}"
	fi


	IMG_NAME="${IMG_NAME}${IMG_FLASH}"

	if [ "x${PACK_SECURE}" != "xnone" -o "x${PACK_SIG}" != "xnone" ]; then
		if [ -f "${ROOT_DIR}/${PACK_PROJECT_PATH}/image/version_base.mk" ]; then
			MAIN_VERION=`awk  '$0~"MAIN_VERSION"{printf"%d", $3}' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/version_base.mk`
		elif [ -f "${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_header.cfg" ]; then
			MAIN_VERION=`grep '"pversion"' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_header.cfg | awk -F \" '{print $4}'`
		fi

		IMG_NAME="${IMG_NAME}_v${MAIN_VERION}.img"
	else
		IMG_NAME="${IMG_NAME}.img"
	fi

	echo "imagename = $IMG_NAME" >> ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg
	echo "" >> ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg
}

function do_common()
{
	cd ${ROOT_DIR}/${PACK_PROJECT_PATH}/image

	busybox unix2dos sys_config${SUFFIX}.fex
	if [ "x${PACK_HOSTOS}" == "xlinux" ] ; then
		${SCRIPT}  sys_config${SUFFIX}.fex > /dev/null
	elif [ "x${PACK_HOSTOS}" == "xwin" ] ; then
		cp ${PACK_TOPDIR}/tools/win-tools/script.dll ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
		${SCRIPT} sys_config${SUFFIX}.bin sys_config${SUFFIX}.fex > /dev/null
	fi
	cp -f   sys_config${SUFFIX}.bin config${SUFFIX}.fex

	busybox unix2dos sys_partition.fex
	if [ "x${PACK_HOSTOS}" == "xlinux" ] ; then
		${SCRIPT}  sys_partition.fex > /dev/null
	elif [ "x${PACK_HOSTOS}" == "xwin" ] ; then
		${SCRIPT} sys_partition.bin sys_partition.fex > /dev/null
	fi

	# Those files for SpiNor. We will try to find sys_partition_nor.fex
	if [ -f sys_partition_nor.fex ];  then

		if [ "x${PACK_SIG}" = "xsecure" ] ; then
			if [ -f sys_partition_nor_sec.patch ]; then
				patch -p1 < sys_partition_nor_sec.patch
			fi
		fi
		# Here, will create sys_partition_nor.bin
		busybox unix2dos sys_partition_nor.fex
		if [ "x${PACK_HOSTOS}" == "xlinux" ] ; then
			${SCRIPT}  sys_partition_nor.fex > /dev/null
		elif [ "x${PACK_HOSTOS}" == "xwin" ] ; then
			${SCRIPT} sys_partition_nor.bin sys_partition_nor.fex > /dev/null
		fi

		${UPDATE_BOOT0} boot0_spinor.fex   sys_config${SUFFIX}.bin SPINOR_FLASH > /dev/null
		${UPDATE_UBOOT} -no_merge u-boot-spinor.fex  sys_config${SUFFIX}.bin > /dev/null

		if [ -f boot_package_nor.cfg -a	x${SUFFIX} == x'' ]; then
			echo "pack boot nor package"
			busybox unix2dos boot_package_nor.cfg
			${DRAGONSECBOOT} -pack boot_package_nor.cfg
			if [ $? -ne 0 ]
			then
				pack_error "dragon pack nor run error"
				exit 1
			fi
			mv boot_package.fex boot_package_nor.fex
		fi
	fi

	${UPDATE_BOOT0} boot0_nand.fex     sys_config${SUFFIX}.bin NAND > /dev/null
	${UPDATE_BOOT0} boot0_nand.fex     sys_config${SUFFIX}.bin NAND > /dev/null
	cp boot0_sdcard.fex boot0_sdcard_noupdate.fex
	${UPDATE_BOOT0} boot0_sdcard.fex	sys_config${SUFFIX}.bin SDMMC_CARD
	${UPDATE_UBOOT} -no_merge u-boot.fex sys_config${SUFFIX}.bin > /dev/null

	${UPDATE_FES1}  fes1.fex           sys_config${SUFFIX}.bin > /dev/null

	if [ -f boot_package.cfg  -a x${SUFFIX} == x'' ]; then
			echo "pack boot package"
			busybox unix2dos boot_package.cfg
			${DRAGONSECBOOT} -pack boot_package.cfg
			if [ $? -ne 0 ]
			then
				pack_error "dragon pack run error"
				exit 1
			fi

			${UPDATE_TOC1}  boot_package.fex           sys_config${SUFFIX}.bin
			if [ $? -ne 0 ]
			then
				pack_error "update toc1 run error"
				exit 1
			fi
	fi

	[ -f env.cfg ] && {
		env_size=4096
		if [ "x${PACK_HOSTOS}" == "xwin" ] ; then
			cp ${PACK_TOPDIR}/tools/win-tools/msys-2.0.dll ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
		fi
		${MKENVIMAGE} -r -p 0x00 -s ${env_size} -o env.fex env.cfg
	}

}

function do_finish()
{
	if [ -f sys_partition_nor.bin ]; then
		if [ "x${PACK_HOSTOS}" == "xlinux" ] ; then
			${UPDATE_MBR} sys_partition_nor.bin 1 sunxi_mbr_nor.fex
		elif [ "x${PACK_HOSTOS}" == "xwin" ] ; then
			cp ${PACK_TOPDIR}/tools/win-tools/update_mbr33.dll ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
			${UPDATE_MBR} sys_partition_nor.bin sunxi_mbr_nor.fex dlinfo.fex 1
		fi

		if [ $? -ne 0 ]; then
			pack_error "update_mbr failed"
			exit 1
		fi
		if [ ${current_rtos_full_img_size} -eq 4 ]; then
			create_rtos_full_img ${LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_4M} ${current_rtos_full_img_size}
		elif [ ${current_rtos_full_img_size} -eq 8 ]; then
			create_rtos_full_img ${LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_8M} ${current_rtos_full_img_size}
		elif [ ${current_rtos_full_img_size} -eq 16 ]; then
			create_rtos_full_img ${LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_16M} ${current_rtos_full_img_size}
		elif [ ${current_rtos_full_img_size} -eq 32 ]; then
			create_rtos_full_img ${LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_32M} ${current_rtos_full_img_size}
		else
			pack_error "full img size ${current_rtos_full_img_size} is not 4/8/16/32M"
			exit 1;
		fi

		cp sys_partition_nor.fex sys_partition_for_dragon.fex

	else
		if [ "x${PACK_HOSTOS}" == "xlinux" ] ; then
			${UPDATE_MBR} sys_partition.bin 4
		elif [ "x${PACK_HOSTOS}" == "xwin" ] ; then
			cp ${PACK_TOPDIR}/tools/win-tools/update_mbr33.dll ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
			${UPDATE_MBR} sys_partition.bin sunxi_mbr.fex dlinfo.fex 4
		fi
		if [ $? -ne 0 ]; then
			pack_error "update_mbr failed"
			exit 1
		fi
		cp sys_partition.fex sys_partition_for_dragon.fex
	fi

	#before r128 usb ready , don't pack dragon format image to save time
	if [ -f sys_partition_for_dragon.fex ]; then
		do_dragon image.cfg sys_partition_for_dragon.fex
	else
		do_dragon image.cfg
	fi

	cd ..
	printf "pack finish\n"

	[ -e ${PACK_TOPDIR}/tools/scripts/.hooks/post-dragon ] &&
		source ${PACK_TOPDIR}/tools/scripts/.hooks/post-dragon
}

function do_dragon()
{
	local partition_file_name="x$2"
	if [ $partition_file_name != "x" ]; then
		echo ====================================
		echo show \"$2\" message
		show_partition_message $2
	fi
	${DRAGON} $@
	echo "-----------${IMG_NAME}----------------"
	if [ $? -eq 0 ]; then
		if [ -e ${IMG_NAME} ]; then
			mv ${IMG_NAME} ../${IMG_NAME}
			echo "----------${image_instruction}----------"
			echo '----------image is at----------'
			echo -e '\033[0;31;1m'
			echo ${ROOT_DIR}/${PACK_PROJECT_PATH}/${IMG_NAME}
			echo -e '\033[0m'
		fi
	fi

}

function image_header_gz()
{
	if [ "x$1" == "x" ]; then
		echo "image_header_gz need param: image_header.cfg"
		exit 1
	fi

	gz_filelist=`grep "\"attr\"" $1 | awk -F '[ ,":]+' '{if(and(strtonum($8), 0x100)) print $6}'`

	if [ -n "$gz_filelist" ]; then
		for file in $gz_filelist; do
			cp -vf $file $file.orig
			gzip -c $file > $file.gz
			cp -vf $file.gz $file
		done
	fi
}

function do_signature_ih()
{
	if [ "x${PACK_HOSTOS}" == "xwin" ] ; then
		cp ${PACK_TOPDIR}/tools/win-tools/msvcp120.dll ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
	fi
	${UPDATE_RTOS_SBOOT} sboot.bin sys_config${SUFFIX}.bin
	if [ $? -ne 0 ]
	then
		pack_error "update sboot run error"
		exit 1
	fi

	# for rsa, 0x3a0(928) = ih(0x80) + tlv(0x20) + 0x100(key_n) + 0x100(key_e) + 0x100(sign)
	# for ecc 0x120(288) = ih(0x80) + tlv(0x20) + 0x40(key_xy) + 0x40(sign)
	sboot_size=`du -b sboot.bin | cut -f1`
	grep "key_type" ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_header.cfg
	if [ $? -eq 0 ]
	then
		key_type=`grep '"key_type"' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_header.cfg | awk -F \" '{print $4}'`
	else
		key_type=rsa
	fi
	if [ "$key_type" == "rsa" ]
	then
		cypto_type_size=928
	elif [ "$key_type" == "ecc" ]
	then
		cypto_type_size=288
	else
		pack_error "key_type error"
		exit 1
	fi
	sboot_align_len=$(((${sboot_size}+${cypto_type_size}+4095)/4096*4096))
	sboot_pad_size=$((${sboot_align_len}-${sboot_size}-${cypto_type_size}))
	dd if=/dev/zero of=sboot.bin bs=1 count=${sboot_pad_size} seek=${sboot_size}

	if [ "x${PACK_HOSTOS}" == "xwin" ] ; then
		cp ${PACK_TOPDIR}/tools/win-tools/msys-2.0.dll ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
	fi

	image_header_gz image_header.cfg

	${MKIMAGE_HEADER} -C 5 -e flash_crypto.ini -c image_header.cfg -s $key_type -k ${ROOT_DIR}/${PACK_PROJECT_PATH}/keys

	if [ $? -ne 0 ]
	then
		pack_error "mkimage_header run error"
		exit 1
	fi

	rename_pack_file

	busybox unix2dos ih_boot_package_nor_sec.cfg
	${DRAGONSECBOOT} -pack ih_boot_package_nor_sec.cfg
	if [ $? -ne 0 ]
	then
		pack_error "dragon pack nor run error"
		exit 1
	fi

	cp -vf boot_package.fex toc1.fex
	cp sboot.bin.pack sboot.bin.pack.bin
	cp sboot.bin.pack toc0.fex

	echo "secure signature ok!"
}

function do_signature()
{
	# merge flag: '1' - merge atf/scp/uboot/optee in one package, '0' - do not merge.
	local merge_flag=0

	printf "prepare for signature by openssl\n"
	cp -v ${PACK_TOPDIR}/board/${PACK_PROJECT_PATH}/sign_config/dragon_toc.cfg dragon_toc.cfg
	if [ $? -ne 0 ]
	then
		pack_error "dragon toc config file is not exist"
		exit 1
	fi

	if [ x${SUFFIX} == x'' ]; then
		${DRAGONSECBOOT} -toc0 dragon_toc.cfg ${ROOT_DIR}/${PACK_PROJECT_PATH}/keys ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/version_base.mk
		if [ $? -ne 0 ]
		then
			pack_error "dragon toc0 run error"
			exit 1
		fi
	fi

	${UPDATE_TOC0}  toc0.fex           sys_config${SUFFIX}.bin
	if [ $? -ne 0 ]
	then
		pack_error "update toc0 run error"
		exit 1
	fi
	if [ x${SUFFIX} == x'' ]; then
		if [ ${merge_flag} == 1 ]; then
			printf "dragon boot package\n"
			${DRAGONSECBOOT} -pack dragon_toc.cfg
			if [ $? -ne 0 ]
			then
				pack_error "dragon boot_package run error"
				exit 1
			fi
		fi

		${DRAGONSECBOOT} -toc1 dragon_toc.cfg ${ROOT_DIR}/${PACK_PROJECT_PATH}/keys \
			${PACK_TOPDIR}/board/${PACK_PROJECT_PATH}/sign_config/cnf_base.cnf \
			${ROOT_DIR}/${PACK_PROJECT_PATH}/image/version_base.mk
		if [ $? -ne 0 ]
		then
			pack_error "dragon toc1 run error"
			exit 1
		fi
	fi

	${UPDATE_TOC1}  toc1.fex           sys_config${SUFFIX}.bin
	if [ $? -ne 0 ]
	then
		pack_error "update toc1 run error"
		exit 1
	fi

	echo "secure signature ok!"
}

#TODO: support handle all *.pack, don't neet to list here
pack_file_list=(
boot0_spinor.fex
boot0_sdcard.fex
boot0_nand.fex
rtos_arm.fex
etf.fex
recovery.fex
psram_arm.fex
rtos_hpsram_arm.fex
rtos_xip_arm.fex
rtos_riscv.fex
rtos_psram_riscv.fex
rtos_hpsram_riscv.fex
rtos_xip_rv.fex
rtos_dsp.fex
rtos_psram_dsp.fex
rtos_hpsram_dsp.fex
rtos_xip_dsp.fex
sboot.bin
tfm.fex
rootkey.fex
config.fex
)

function rename_pack_file()
{
	for file in ${pack_file_list[@]} ; do
		[ -f ${file}.pack ] && {
			mv -v ${file} ${file}.nopack
			cp -v ${file}.pack ${file}
		}
	done
}

function check_config_arm_addr()
{
	ARM_ADDR=`grep '"rtos_arm.fex"' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_header.cfg | sed 's/\"//g' | sed 's/,//g' | awk '{print $8}'`
	CONFIG_ADDR=`grep '"config.fex"' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image_header.cfg | sed 's/\"//g' | sed 's/,//g' | awk '{print $8}'`
	ARM_LONGTH=`printf '%d' $ARM_ADDR`
	CONFIG_LONGTH=`printf '%d' $CONFIG_ADDR`
	CONFIG_SIZE=`ls -nl config.fex | awk '{print $5}'`
	if [ $[ `expr $CONFIG_LONGTH + $CONFIG_SIZE` ] -gt $ARM_LONGTH ] ; then
		pack_error "config.fex is too large!"
		pack_error "pack error, please expand arm start address!"
		exit 1
	fi
}

function do_pack_rtos()
{
	printf "packing for rtos\n"
	if [ "x${PACK_CHIP}" = "xsun20iw2p1" ] ; then
		check_config_arm_addr
	fi

	# if [ "x${storage_type}" = "x3" ] ; then
		# boot_package/toc1 limit to 4M，but rtos may large then 4M
		rm -f rtos_pkg_nor.fex
		rm -f rtos_pkg.fex
		rm -f rtos_toc1.fex
	# fi

	if [ "x${PACK_SIG}" = "xsecure" ] ; then
		echo "secure"
		if [ "x${PACK_CHIP}" = "xsun20iw2p1" ] ; then
			do_signature_ih
		else
			do_signature
		fi
		if [ "x${storage_type}" = "x5" ] ; then
			${UPDATE_RTOS} --image freertos-gz.fex \
				--output freertos-gz-update.fex
			if [ $? -ne 0 ]; then
				pack_error "add rtos header error!"
				exit 1
			fi
			ln -s freertos-gz-update.fex rtos_pkg_nor.fex
			ln -s freertos-gz-update.fex rtos_pkg.fex
		fi
		if [ "x${storage_type}" = "x3" ] ; then
			mv toc1.fex rtos_toc1.fex
			ln -s rtos_toc1.fex rtos_pkg_nor.fex
			ln -s rtos_toc1.fex rtos_pkg.fex
			# keep small toc1.fex to make uboot happy, won't write to flash
			dd if=rtos_toc1.fex of=toc1.fex bs=1k count=32
		fi
	elif [ "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		echo "prev_refurbish"
		if [ "x${PACK_CHIP}" = "xsun20iw2p1" ] ; then
			do_signature_ih
		else
			do_signature
		fi
		if [ "x${storage_type}" = "x5" ] ; then
			${UPDATE_RTOS} --image freertos-gz.fex \
				--output freertos-gz-update.fex
			if [ $? -ne 0 ]; then
				pack_error "add rtos header error!"
				exit 1
			fi
			ln -s freertos-gz-update.fex rtos_pkg_nor.fex
			ln -s freertos-gz-update.fex rtos_pkg.fex
		fi
		if [ "x${storage_type}" = "x3" ] ; then
			mv toc1.fex rtos_toc1.fex
			ln -s rtos_toc1.fex rtos_pkg_nor.fex
			ln -s rtos_toc1.fex rtos_pkg.fex
			# keep small toc1.fex to make uboot happy, won't write to flash
			dd if=rtos_toc1.fex of=toc1.fex bs=1k count=32
		fi
	else
		echo "normal"
		if [ "x${storage_type}" = "x5" ] ; then
			${UPDATE_RTOS} --image freertos-gz.fex \
				--output freertos-gz-update.fex
			if [ $? -ne 0 ]; then
				pack_error "add rtos header error!"
				exit 1
			fi
			ln -s freertos-gz-update.fex rtos_pkg_nor.fex
			ln -s freertos-gz-update.fex rtos_pkg.fex
		fi

		if [ "x${storage_type}" = "x3" ] ; then
			# new image header operation
			if [ "x${PACK_CHIP}" == "xsun20iw2p1" ]; then
				if [ "x${PACK_HOSTOS}" == "xwin" ] ; then
					cp ${PACK_TOPDIR}/tools/win-tools/msys-2.0.dll ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
				fi

				image_header_gz image_header.cfg

				${MKIMAGE_HEADER} -c image_header.cfg
				if [ $? -ne 0 ]
				then
					pack_error "mkimage run error"
					exit 1
				fi

				rename_pack_file

			fi
			mv boot_package_nor.fex rtos_pkg_nor.fex
			mv boot_package.fex rtos_pkg.fex
			# keep small boot_package.fex/boot_package_nor.fex to make uboot happy, won't write to flash
			dd if=rtos_pkg_nor.fex of=boot_package_nor.fex bs=1k count=32
			dd if=rtos_pkg.fex of=boot_package.fex bs=1k count=32

			echo "====for card product===="
			busybox unix2dos boot_package.cfg
			${DRAGONSECBOOT} -pack boot_package.cfg
		else
			# nand
			# new image header operation
			if [ x"${PACK_CHIP}" == x"sun20iw2p1" ]; then
				if [ "x${PACK_HOSTOS}" == "xwin" ] ; then
					cp ${PACK_TOPDIR}/tools/win-tools/msys-2.0.dll ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
				fi

				image_header_gz image_header.cfg

				${MKIMAGE_HEADER} -c image_header.cfg
				if [ $? -ne 0 ]
				then
					pack_error "mkimage run error"
					exit 1
				fi

				rename_pack_file

				busybox unix2dos boot_package.cfg
				${DRAGONSECBOOT} -pack boot_package.cfg
				if [ $? -ne 0 ]
				then
					pack_error "dragon pack nor run error"
					exit 1
				fi
			fi
		fi
	fi
}

function create_rtos_full_img()
{
		#rtos use uboot-2018, so not use mbr, but gpt
		gpt_file=sunxi_gpt_nor.fex
		mbr_source_file=sunxi_mbr_nor.fex
		#rtos logic start is 112K
		#LOGIC_START=112
		#r128 rtos logic start is 144K (64*2+16)
		if [ "x${PACK_CHIP}" = "xsun20iw2p1" ]; then
			LOGIC_START=128
		else
			LOGIC_START=112
		fi
		if [ "x${PACK_SIG}" = "xsecure" ] ; then
			boot0_file_name=toc0.fex
			redund_boot0_file_name=toc0.fex
			if [ -f boot0_offline_secure_${PACK_CHIP}.fex ]; then
				#this is normal boot file, use it to burn secure bit.
				redund_boot0_file_name=boot0_offline_secure_${PACK_CHIP}.fex
			fi
			full_rtos_img_name=rtos_${2}Mnor_s.bin
		else
			boot0_file_name=boot0_spinor.fex
			redund_boot0_file_name=boot0_spinor.fex
			full_rtos_img_name=rtos_${2}Mnor.bin
		fi

		echo ----------------mbr convert to gpt start---------------------
		${MBR_CONVERT_TO_GPT} --source ${mbr_source_file} \
			               --out ${gpt_file} \
			               --input_logic_offset $((${LOGIC_START} * 1024 / 512 )) \
			               --input_flash_size ${2}
		echo ----------------mbr convert to gpt end---------------------

		#rtos have not uboot, rtos use gpt
		${MERGE_FULL_RTOS_IMG} --out ${full_rtos_img_name} \
		                    --boot0 ${boot0_file_name} \
		                    --redund_boot0 ${redund_boot0_file_name} \
		                    --mbr ${gpt_file} \
		                    --logic_start ${LOGIC_START} \
		                    --partition sys_partition_nor.bin \
		                    --UDISK_partition_size ${1} \
		                    --total_image_size ${2}
		if [ $? -ne 0 ]; then
			pack_error "merge_full_rtos_img failed"
			exit 1
		else
			echo -e '\033[0;31;1m'
			echo ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/${full_rtos_img_name}
			echo -e '\033[0m'
			if [ "x${PACK_HOSTOS}" == "xlinux" ] ; then
				${CUT_TRAILING_BYTES} ${full_rtos_img_name} -c ff
			elif [ "x${PACK_HOSTOS}" == "xwin" ] ; then
				local full_image_file=$(cygpath -w ${full_rtos_img_name})
				${CUT_TRAILING_BYTES} ${full_image_file/\\/\\\\} -c ff
			fi
		fi
}

function prepare_for_4Mnor()
{
	cd ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
	echo "making data image for 4M nor"
	make_pstore_image sys_partition_nor.fex ${LOGICAL_PARTS_KB_FOR_4M} LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_4M "cut_zero"
	make_data_image sys_partition_nor.fex ${LOGICAL_PARTS_KB_FOR_4M} LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_4M "cut_zero"
	sed -i 's/\(imagename = .*\)_[^_]*nor/\1_4Mnor/g' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg
	IMG_NAME=$(awk '{if($3~/^rtos.*img$/)print$3}' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg)
	current_rtos_full_img_size=4
}

function prepare_for_8Mnor()
{
	cd ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
	echo "making data image for 8M nor"
	make_pstore_image sys_partition_nor.fex ${LOGICAL_PARTS_KB_FOR_8M} LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_8M "cut_zero"
	make_data_image sys_partition_nor.fex ${LOGICAL_PARTS_KB_FOR_8M} LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_8M "cut_zero"
	sed -i 's/\(imagename = .*\)_[^_]*nor/\1_8Mnor/g' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg
	IMG_NAME=$(awk '{if($3~/^rtos.*img$/)print$3}' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg)
	current_rtos_full_img_size=8
}

function prepare_for_16Mnor()
{
	cd ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
	echo "making data image for 16M nor"
	make_pstore_image sys_partition_nor.fex ${LOGICAL_PARTS_KB_FOR_16M} LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_16M "cut_zero"
	make_data_image sys_partition_nor.fex ${LOGICAL_PARTS_KB_FOR_16M} LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_16M "cut_zero"
	make_rtos_reserve_image sys_partition_nor.fex ${LOGICAL_PARTS_KB_FOR_16M} rtosA reserve
	sed -i 's/\(imagename = .*\)_[^_]*nor/\1_16Mnor/g' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg
	IMG_NAME=$(awk '{if($3~/^rtos.*img$/)print$3}' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg)
	current_rtos_full_img_size=16
}

function prepare_for_32Mnor()
{
	cd ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
	echo "making data image for 32M nor"
	make_data_image sys_partition_nor.fex ${LOGICAL_PARTS_KB_FOR_32M} LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_32M "cut_zero"
	make_rtos_reserve_image sys_partition_nor.fex ${LOGICAL_PARTS_KB_FOR_32M} rtosA reserve
	sed -i 's/\(imagename = .*\)_[^_]*nor/\1_32Mnor/g' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg
	IMG_NAME=$(awk '{if($3~/^rtos.*img$/)print$3}' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg)
	current_rtos_full_img_size=32
}

function prepare_for_128Mnand()
{
	cd ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
	echo "making data image for 128M nand"
	make_data_image sys_partition.fex ${LOGICAL_PARTS_KB_FOR_128Mnand} LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_128Mnand "cut_zero"
	sed -i 's/\(imagename = .*\)_[^_]*nand/\1_128Mnand/g' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg
	IMG_NAME=$(awk '{if($3~/^rtos.*img$/)print$3}' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg)
	current_rtos_full_img_size=0 #not support now
}

function prepare_for_256Mnand()
{
    cd ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
    echo "making data image for 256M nand"
    make_data_image sys_partition.fex ${LOGICAL_PARTS_KB_FOR_256Mnand} LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_256Mnand "cut_zero"
    sed -i 's/\(imagename = .*\)_[^_]*nand/\1_256Mnand/g' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg
    IMG_NAME=$(awk '{if($3~/^rtos.*img$/)print$3}' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg)
    current_rtos_full_img_size=0 #not support now
}

function prepare_for_512Mnand()
{
	cd ${ROOT_DIR}/${PACK_PROJECT_PATH}/image
	echo "making data image for 512M nand"
	make_data_image sys_partition.fex ${LOGICAL_PARTS_KB_FOR_512Mnand} LOGICAL_UDISK_PARTS_KB_REMAIN_FOR_512Mnand "cut_zero"
	sed -i 's/\(imagename = .*\)_[^_]*nand/\1_512Mnand/g' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg
	IMG_NAME=$(awk '{if($3~/^rtos.*img$/)print$3}' ${ROOT_DIR}/${PACK_PROJECT_PATH}/image/image.cfg)
	current_rtos_full_img_size=0 #not support now
}

function part_offset()
{
	[ -f sunxi_mbr_nor.fex ] || {
		${UPDATE_MBR} sys_partition_nor.bin 1 sunxi_mbr_nor.fex  2> /dev/null
	}
	if [ x"$1" = x"boot0" ]; then
		#boot0 in offset 0
		echo $((0*512))
	elif [ x"$1" = x"boot0-redund" ]; then
		#boot0-redund in offset 64k
		echo $((128*512))
	elif [ x"$1" = x"gpt" ]; then
		#gpt in offset 128k
		echo $((256*512))
	else
		offset=$(${PARSER_MBR} sunxi_mbr_nor.fex get_offset_by_name $1)
		if [ x"$offset" = x"0" ]; then
			#can not find this part, return 0
			echo 0
		else
			echo $(($offset*512+256*512))
		fi
	fi
}

do_prepare
do_common
do_pack_${PACK_PLATFORM}

if [ "x${PACK_MODE}" = "xdump" ] ; then
	do_finish
elif [ x"${storage_type}" = x"3" ]; then
	if [ "x${PACK_BOARD%%_*}" == "xr128s1" ]; then
	    prepare_for_8Mnor
	else
	    prepare_for_16Mnor
	fi
	do_finish
elif [ x"${storage_type}" = x"5" ]; then
	prepare_for_128Mnand
	do_finish
else
	do_finish
fi

