#!/bin/bash
#
# scripts/mkcommon.sh
# (c) Copyright 2013
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# James Deng <csjamesdeng@allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

BR_SCRIPTS_DIR=$(cd $(dirname $0) && pwd)
BUILD_CONFIG=$BR_SCRIPTS_DIR/../.buildconfig
source ${BR_SCRIPTS_DIR}/mkcmd.sh

#load export value
if [ -f ${BUILD_CONFIG} ]; then
	. ${BUILD_CONFIG}
fi

# ./build.sh
if [ $# -eq 0 ]; then
	mk_defconfig
	mk_lichee
else
	while [ $# -gt 0 ]; do
		case "$1" in
		config*)
			. ${BR_SCRIPTS_DIR}/mksetup.sh
			#update env for .buildconfig
			. ${BUILD_CONFIG}
			mk_defconfig
			mk_install_xcc
			break;
			;;
		menuconfig*)
			export PATH=${LICHEE_CUR_PATH};
			mk_defconfig && make menuconfig && mk_savedefconfig
			break;
			;;
		tools*)
			mk_delete_xcc
			mk_install_xcc
			break;
			;;
		clean*)
			mk_clean
			#mk_delete_xcc
			#rm -rf ${BUILD_CONFIG}
			break;
			;;
		*) ;;
		esac;
		shift;
	done
fi



