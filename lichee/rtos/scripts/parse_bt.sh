#!/bin/bash

help_info()
{
	echo "Usage: ./parse_bt.sh <syms> <bt_log>"
	echo -e "  - syms\t path to rt_system.syms"
	echo -e "  - bt_log\t the backtrace log file"
}

if [ $# -ne 2 ]; then
	help_info
	exit
fi

grep 'SYMBOL TABLE:' $1 > /dev/null
if [ $? -ne 0 ]; then
	echo "symbol table file $1 seems wrong!!!"
fi

sort $1 > .sort.rt_system.syms

while read line
do
	enter_addr=`echo $line | grep '<0x' | awk -F '[<>]' '{print toupper($2)}' | sed 's/0X//g'`
	from_addr=`echo $line | grep '<0x' | awk -F '[<>]' '{print toupper($4)}' | sed 's/0X//g'`

	if [ "x$enter_addr" = "x" ]; then
		echo $line
		continue
	fi

	old_enter_addr=$enter_addr
	enter_offset=0
	while true
	do
		grep -i $enter_addr .sort.rt_system.syms > /dev/null
		if [ $? -ne 0 ]; then
			enter_offset=`echo "obase=16;ibase=16;$enter_offset+4" | bc`
			enter_addr=`echo "obase=16;ibase=16;$old_enter_addr-$enter_offset" | bc`
		else
			enter_func=`grep -i $enter_addr .sort.rt_system.syms | awk '{print $6}'`
			break
		fi
	done

	old_from_addr=$from_addr
	from_offset=0
	while true
	do
		grep -i $from_addr .sort.rt_system.syms > /dev/null
		if [ $? -ne 0 ]; then
			from_offset=`echo "obase=16;ibase=16;$from_offset+4" | bc`
			from_addr=`echo "obase=16;ibase=16;$old_from_addr-$from_offset" | bc`
		else
			from_func=`grep -i $from_addr .sort.rt_system.syms | awk '{print $6}'`
			break
		fi
	done

	echo "enter [<0x$old_enter_addr>] ($enter_func+0x$enter_offset) from [<0x$old_from_addr>] ($from_func+0x$from_offset)"

done < $2

rm -rf .sort.rt_system.syms
