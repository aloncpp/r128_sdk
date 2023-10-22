# -*- coding: utf-8 -*-
#!/usr/bin/env python

# 可用版本 V1.9 时间: 2022年5月12日
# author: lijiajian@allwinnertech.com
# 计算出来的大小和size出来的一致
# 目前仅支持rtos, melis使用kbuild方式编译,未支持

import os
import sys
import argparse
import re
import subprocess

undown_segment = []
attch_name_group = {}
text_name_group = []
data_name_group = []
bss_name_group = []
ignore_fill_addr = [] #需要忽略一些填充地址,如stack段大小是fill来的,如果不忽略改fil,计算时会算到前一个变量上

elf_addr_range = {}

# 需要特殊处理的段
special_groups = {
    # 常用段名
    '.text' : 'text',
    '.rodata' : 'text',
    '.data' : 'data',
    '.bss' : 'bss',
    # 这里特殊处理下 COMMON 段,该段属于Bss,
    # 该段只存在于编译阶段,最后会链接到bss段
    'COMMON' : 'bss',
    # rtos 特有段
    '.freertos_vector': 'text',
    '.start' : 'text',
    '.isr_vector': 'text',
    '.cpu_text': 'text',  # r128 有这个段
    '.nonxip_text': 'text',         # r128 有这个段
    # 重定位表, .got保存引用的外部变量. .got.plt保存引用的外部函数(静态链接用)
    '.got': 'data',
    # 重定位表, .rel.xxx 保存xxx重定位信息.如.rel.text (动态链接用)
    '.rel': 'data',
    # 存放ARM用于栈回溯的一些信息,.ARM.extab是exidx的附属结构
    '.ARM.exidx': 'text',
    '.ARM.extab': 'text',
}

# 需要转换的段名,如FSymTab段,在map中名称是 FSymTab. 在elf段表中,段名是 .FSymTab
special_segments_name = {
    'FSymTab': '.FSymTab',
    'ttcall': '.ttcall',
}

# Archive member segment record reference other function
segment_start = [
    "Linker script and memory map",
    "链结器命令稿和内存映射"
    ]
segment_end = "OUTPUT\(/"

parser = argparse.ArgumentParser()
parser.add_argument("map",help = "map file directory e.g. rt_system.map")
parser.add_argument("elf",help = "elf file e.g rt_system.elf")
parser.add_argument("--out",help = "output file name")
parser.add_argument("--debug", "-d", action='store_true', help="show more info", default=False)

args = parser.parse_args()
cwd = os.getcwd()

path = args.map
elf_file = args.elf
dirent = os.path.dirname(path)
if len(dirent) == 0:
    dirent = os.getcwd()
print("Map file = " + path)
print("elf file = " + elf_file)

debug = args.debug
txt_file = None

if args.out:
    print("out = " + args.out)
    txt_file = args.out

def clean_tempfile():
    tempfile = [
        'temp.txt', '.temp.txt', '.out.txt', 'out.txt', 'nomatch.txt',
        'text.txt', 'data.txt', 'bss.txt', 'fill.txt', 'out.txt', 'err.txt'
    ]

    for item in tempfile:
        if os.path.exists(item):
            os.remove(item)

def print_head(out):
    line = "{: <27}{: ^17} |{: >17} {: >17} {: >17} | 路径".format("文件名",
                "total", "text", "data", "bss")
    if out:
        out.write(line + '\n')
    else:
        print(line)

def print_line(out, all, name, total, data, text, bss, total_data, total_text, total_bss, dir):
    percent_all = "%3.2f%%" %( float(total * 100)/all )
    percent_text = None
    percent_data = None
    percent_bss = None
    if total_text > 0:
        percent_text = "%3.2f%%" %( float(text * 100)/total_text )
    if total_data > 0:
        percent_data = "%3.2f%%" %( float(data * 100)/total_data )
    if total_bss > 0:
        percent_bss = "%3.2f%%" %( float(bss * 100)/total_bss )
    if len(dir) > 50:
        dir = '...' + dir[-50:]

    # line = "{: <26}{: ^10}|{: >8}({: >6}) {: >8}({: >6}) {: >8}({: >6}) | {: <8} | {}".format(name,
    #             total, text, percent_text, data, percent_data,
    #             bss, percent_bss, percent_all, dir)
    line = "{: <30}{: >8}({: >7}) |".format(name, total, percent_all)
    if total_text > 0 and text > 0:
        line += "{: >8}({: >7}) ".format(text, percent_text)
    else:
        line += "{: >17} ".format(text)

    if total_data > 0 and data > 0:
        line += "{: >8}({: >7}) ".format(data, percent_data)
    else:
        line += "{: >17} ".format(data)

    if total_bss > 0 and bss > 0:
        line += "{: >8}({: >7}) ".format(bss, percent_bss)
    else:
        line += "{: >17} ".format(bss)

    line += "| {}".format(dir)
    if out:
        out.write(line + '\n')
    else:
        print(line)

def print_subobj(out, name, file_size, data, text, bss, total_data, total_text, total_bss):
    total = data + text + bss
    percent_all = "%3.2f%%" %( float(total * 100)/file_size )
    if total_text > 0:
        percent_text = "%3.2f%%" %( float(text * 100)/total_text )
    if total_data > 0:
        percent_data = "%3.2f%%" %( float(data * 100)/total_data )
    if total_bss > 0:
        percent_bss = "%3.2f%%" %( float(bss * 100)/total_bss )

    # line = "    {: <22}{: ^10}|{: >8}({: >6}) {: >8}({: >6}) {: >8}({: >6}) ".format(name, total,
    #             text, percent_text, data, percent_data, bss, percent_bss)
    line = " |--{: <26}{: >8}({: >7}) |".format(name, total, percent_all)
    if total_text > 0 and text > 0:
        line += "{: >8}({: >7}) ".format(text, percent_text)
    else:
        line += "{: >17} ".format(text)

    if total_data > 0 and data > 0:
        line += "{: >8}({: >7}) ".format(data, percent_data)
    else:
        line += "{: >17} ".format(data)

    if total_bss > 0 and bss > 0:
        line += "{: >8}({: >7}) ".format(bss, percent_bss)
    else:
        line += "{: >17} ".format(bss)

    if out:
        out.write(line + '\n')
    else:
        print(line)

# 解析 map 文件,并生成中间文件
# 会产生3个中间文件:
#   1. temp.txt    将目标行处理成一行后的map文件
#   2. .temp.txt   仅留下目标行的文件
#   3. fill.txt    记录因对齐而额外占用的空间的文件
#   4. nomatch.txt 所以非目标行(debug用)
#   5. .out.txt    文件,初始整理后的数据
def find_segment(path,start, end):
    enabled = False

    # 寻找匹配1 不匹配2 的行
    pattern1 = re.compile(r'^[ ]*\.([ a-zA-z\d\._]+)[\r]?$')
    pattern2 = re.compile(r'^\.(\S+) +0x\d+')
    # 匹配没换行的匹配行
    pattern3 = re.compile(r'^[ ]?([a-zA-z\d\.]+)[ ]+0x[\da-fA-f]+[ ]+(0x[\da-fA-f]+)[ ]+(\S+)\.o([\)]?)$')
    # 匹配填充的行
    patter4 = re.compile(r'^ *\*fill\* +(0x[a-z\d]+) +(0x[a-z\d]+) *\S*$')
    # 特殊段 linker stubs
    pattern5 = re.compile(r'^[ ]?([a-zA-z\d\.]+)[ ]+0x[\da-fA-f]+[ ]+(0x[\da-fA-f]+)[ ]+(linker stubs *\S*)$')

    # 第一次处理 找到换行了的目标行 将其处理成一行
    ff = open("temp.txt", 'w+')
    with open(path, 'rt') as f:
        for line in f:
            # line = line.strip()
            if len(line) < 5 or len(line) == '\n':
                continue
            if end in line:
                print("Find end line")
                break
            if enabled == True:
                match1 = pattern1.search(line)
                match2 = pattern2.search(line)
                if match1 and not match2:
                    ff.write(line.replace('\n', '').strip())
                else:
                    line += '\n'
                    line.strip()
                    ff.write(line)
            for item in start:
                if item in line:
                    # print("Find start line")
                    enabled = True
    ff.close()

    if enabled == False:
        print("ERROR:Can't find target segment info.\n")
        exit(0)

    fff = open("nomatch.txt", 'w')
    # 第二次处理 只留下目标行
    ff = open(".temp.txt", 'w+')
    with open('temp.txt', 'r') as f:
        for line in f:
            if len(line) < 5 or len(line) == '\n':
                continue
            match = pattern3.search(line)
            match2 = pattern5.search(line)
            if match:
                seg_name = match.group(1)
                if seg_name.startswith('.debug'):
                    continue
                if seg_name.startswith('.comment'):
                    continue
                if seg_name.startswith('.ARM.attributes'):
                    continue
                if seg_name.startswith('.riscv.attributes'):
                    continue
                ff.write(line)
            elif match2:
                ff.write(line)
            else:
                fff.write(line)
    ff.close()
    fff.close()

    # 再次处理,获取 map 文件中的填充占用的内存(因对齐需要而产生)
    ff = open("fill.txt", 'w+')
    with open(path, 'rt') as f:
        for line in f:
            match = patter4.match(line)
            if match:
                ff.write(line)
    ff.close()

def print_data(out, name, major, seg, size):
    line = "{: <50}{: <10}{: <10}{: <}".format(
        name, major, size, seg)
    # print(line)
    if out:
        out.write(line + '\n')

def save_line(out, name, minor, addr, major, size, path):
    line = "{: <50} {: <50} {: <20} {: <10} {: <10} {: <}".format(
        name, minor, addr, major, size, path)
    # print(line)
    if out:
        out.write(line + '\n')

def add_addr_range_item(l, major, addr, length):

    for i in range(len(l[major])):
        start = l[major][i][0]
        end = l[major][i][1]
        # 在前面
        if (addr + length) == start:
            l[major][i][0] = addr
            return l
        # 在后面
        if end == addr:
            l[major][i][1] += length
            return l
    ll = []
    ll.append(addr)
    ll.append(addr + length)
    l[major].append(ll)

def get_addr_major_type(ranges, addr, length):
    for k,v in ranges.items():
        for i in range(len(v)):
            start = v[i][0]
            end = v[i][1]
            if addr >= start and (addr + length) <= end:
                return k
    return None

# 使用 readelf 解析elf文件,获取信息
def update_segment_group():
    global elf_file
    global text_name_group
    global data_name_group
    global bss_name_group
    global elf_addr_range

    elf_addr_range.clear()
    elf_addr_range['bss'] = []
    elf_addr_range['data'] = []
    elf_addr_range['text'] = []
    text_size = 0
    data_size = 0
    bss_size = 0
    s = subprocess.Popen("readelf -SW " + elf_file, shell=True, stdout=subprocess.PIPE)
    out = s.communicate()[0].decode()
    dst = re.split('\n', out)
    for item in dst:
        match = re.match(r'^ +\[ *\d+\] +(\.[\S\.]+) * ([\S\d]+) *([\da-z]+) +[\da-z]+ +([\da-z]+) +\d+ +([A-Za-z]+)', item)
        if match:
            seg_name = match.group(1)
            seg_type = match.group(2)
            addr = int('0x' + match.group(3), 16)
            size = int('0x' + match.group(4), 16)
            flag = match.group(5)

            if 'A' in flag and 'W' not in flag:
                text_name_group.append(seg_name)
                text_size += size
                add_addr_range_item(elf_addr_range, 'text', addr, size)
                continue
            if 'A' in flag and 'W' in flag and seg_type != 'NOBITS':
                data_name_group.append(seg_name)
                data_size += size
                add_addr_range_item(elf_addr_range, 'data', addr, size)
                continue
            if 'A' in flag and 'W' in flag and seg_type == 'NOBITS':
                bss_name_group.append(seg_name)
                bss_size += size
                add_addr_range_item(elf_addr_range, 'bss', addr, size)
                continue

def try_to_match_segment(name, addr, size):
    global text_name_group
    global data_name_group
    global bss_name_group
    global special_groups
    global elf_addr_range
    global undown_segment
    global special_segments_name

    if name in special_segments_name.keys():
        name = special_segments_name[name]

    for item in text_name_group:
        if name.startswith(item):
            attch_name_group[item] = True
            return 'text'
    for item in data_name_group:
        if name.startswith(item):
            attch_name_group[item] = True
            return 'data'
    for item in bss_name_group:
        if name.startswith(item):
            attch_name_group[item] = True
            return 'bss'

    for k, v in special_groups.items():
        if name.startswith(k):
            return v

    # print('Undown segment:%s' % name)
    if name not in undown_segment:
        undown_segment.append(name)
    return None

def is_needed_match(match, name):
    for item in match:
        if name.startswith(item):
            return True
    return False

def get_segment_major(seg_name, addr, size):
    # directive to match
    ret = try_to_match_segment(seg_name, addr, size)
    return ret

# 处理中间文件,整理数据并输出到 .out.txt (未根据fill.txt调整大小)
def parser_target():
    global cwd
    global ignore_fill_addr
    fill_dict = {}

    os.chdir(cwd)

    err = open("nomatch.txt", 'w')
    ff = open(".out.txt", 'w')    
    # .bss.dump_debug_info 0x0000000040321704 0x4 /home/
    with open(".temp.txt", 'r') as f:
        for line in f:
            lists = line.split()
            if 'linker stubs' not in line:
                if len(lists) != 4:
                    print("error line: %s" % line)
                    exit
                seg_name = lists[0]
                addr = lists[1]
                size = lists[2]
                dir = os.path.dirname(lists[3])
                objname = os.path.basename(lists[3])
                path = os.path.relpath(dir)
                major = get_segment_major(seg_name, addr, size)
                if not major:
                    err.write(line)
                    continue
                if int(size, 16) > 0:
                    save_line(ff, objname, seg_name, addr, major, size, path)
            else:
                objname = 'linker(stubs)'
                seg_name = lists[0]
                addr = lists[1]
                size = lists[2]
                major = 'text'
                path = 'Null'
                if not major:
                    err.write(line)
                    continue
                if int(size, 16) > 0:
                    save_line(ff, objname, seg_name, addr, major, size, path)
    ff.close()
    err.close()

# 根据 fill.txt 调整变量占用的地址大小,输出到 out.txt
def adjust_targer_file():
    global cwd
    global ignore_fill_addr
    fill_dict = {}

    os.chdir(cwd)

    err = open("nomatch.txt", 'w')
    ff = open("out.txt", 'w')    

    # 生成填充字典
    with open("fill.txt", 'r') as f:
        for line in f:
            group = line.split()
            addr = int(group[1], 16)
            size = int(group[2], 16)
            fill_dict[addr] = size

    # 取出需要忽略的fill 地址
    for item in ignore_fill_addr:
        if item in fill_dict.keys():
            # print('remove fill 0x%x' % item)
            del fill_dict[item]

    # format: obj_name seg_name addr major size path
    with open(".out.txt", 'r') as f:
        for line in f:
            lists = line.split()
            if len(lists) != 6:
                print("error line: %s" % line)
                exit
            objname = lists[0]
            seg_name = lists[1]
            addr = lists[2]
            major = lists[3]
            size = lists[4]
            path = lists[5]

            # 查询填充字典,是否填充了内存,主要是结束地址
            while True:
                end_addr = int(addr, 16) + int(size, 16)
                if end_addr in fill_dict.keys():
                    # print('adjust %s:%s:%s -> 0x%x' % (seg_name, addr, size, int(size, 16) + fill_dict[end_addr]))
                    size = hex(int(size, 16) + fill_dict[end_addr])
                    del fill_dict[end_addr]
                else:
                    break

            if int(size, 16) > 0:
                save_line(ff, objname, seg_name, addr, major, size, path)
    ff.close()
    err.close()

# 寻找一些在 elf段表,但是没匹配上的段(它们可能没有对应的.o文件,如stack段)
def print_needed_segments():
    global text_name_group
    global data_name_group
    global bss_name_group
    global attch_name_group
    global debug

    nomatch = []
    # 获取没匹配上,但需要的段
    for item in text_name_group:
        if item not in attch_name_group.keys():
            nomatch.append(item)
    for item in data_name_group:
        if item not in attch_name_group.keys():
            nomatch.append(item)
    for item in bss_name_group:
        if item not in attch_name_group.keys():
            nomatch.append(item)
    if debug == True:
        print('未搜索到的段:')
        print(nomatch)
    return nomatch

# 将一些需要但是没匹配上的段保存到out.txt中 如 .stack 不链接文件 但是占用空间
def find_needed_segment(nomatch):
    global undown_segment
    global ignore_fill_addr

    enabled = False

    # 匹配特殊段 的行 如 .stack 不链接文件 占用空间由fill得来
    # .text .bss .rodata .data 除外
    pattern = re.compile(r'^ ?(\.\S+) +(0x[\da-f]+) +(0x[\da-f]+)$')
    # 寻找特殊段
    ff = open(".out.txt", 'a')
    with open('temp.txt', 'r') as f:
        for line in f:
            if len(line) < 1 or len(line) == '\n':
                continue
            match = pattern.search(line)
            if match:
                seg_name = match.group(1)
                addr = match.group(2)
                size = match.group(3)
                r = is_needed_match(nomatch, seg_name)
                if r == False:
                    continue

                major = get_segment_major(seg_name, addr, size)
                if major == None:
                    continue

                objname = major
                path = 'Null'
                # path = seg_name
                if int(size, 16) > 0:
                    save_line(ff, objname, seg_name, addr, major, size, path)
                # 从 undown_segment 列表删除
                # print('%s -> %s' % (seg_name, major))
                if seg_name in undown_segment:
                    undown_segment.remove(seg_name)
                # 如果其地址在fill.txt中,将其删除
                ignore_fill_addr.append(int(addr, 16))
                
    ff.close()

# 计算出实际占用的地址 因为会出现变量复用地址的情况,map记录的大小不一定就占用了这么大
# addrs = [[start, end], [start, end]]
# 返回实际的大小
def get_read_size(addrs, a, s):
    addr = int(a, 16)
    size = int(s, 16)
    if size <= 0:
        return 0

    for i in range(len(addrs)):
        start = addrs[i][0]
        end = addrs[i][1]
        # 情况1 完全复用
        if addr >= start and (addr + size) <= end:
            return 0
        # 情况2 复用一部分 前面重合
        elif addr < start and (addr + size) >= start and (addr + size) <= end:
            realsize = start - addr
            # print('head [0x%x - 0x%x] -> [0x%x - 0x%x] (%s:%s)' % (start, end, addr, end, a, s))
            addrs[i][0] = addr
            return realsize
        # 情况3 复用一部分 后面重复
        elif addr > start and addr <= end and (addr + size) > end:
            realsize = addr + size - end
            # print('rear [0x%x - 0x%x] -> [0x%x - 0x%x] (%s:%s)' % (start, end, start, addr + size, a, s))
            addrs[i][1] = addr + size
            return realsize

    # 情况4 完全不重合 添加一个新的item
    item = []
    item.append(addr)
    item.append(addr + size)
    # print('\n\n origin:')
    # for it in addrs:
    #     print('0x%x - 0x%x' % (it[0], it[1]))
    # print('new item:')
    # print('\t0x%x - 0x%x' % (item[0], item[1]))
    addrs.append(item)
    return size

# 读取最后整理好的数据,并进行排序输出
def statistics_target():
    global txt_file

    output_libs = {}    # 记录目标文件的大小,key=文件名, value=大小
    output_data = {}    # 记录目标文件的数据段大小,key=文件名, value=大小
    output_text = {}    # 记录目标文件的代码段大小,key=文件名, value=大小
    output_bss = {}     # 记录目标文件的bss段大小,key=文件名, value=大小
    output_dir = {}     # 记录目标文件的路径,key=文件名, value=路径
    output_libs_sub = {}

    address = []        # 因为会出现变量复用地址的情况,需要额外来记录
    text_address = []
    data_address = []
    bss_address = []
    total_size = 0
    total_text = 0
    total_bss = 0
    total_data = 0

    pattern1 = re.compile(r'^(\S+\.a)\((\S+\.o)\)')
    pattern2 = re.compile(r'^\S+\.a')

    with open("out.txt", 'r') as f:
        for line in f:
            # name, minor, major, size, path
            group = line.split()
            name = group[0]
            minor = group[1]
            addr = group[2]
            major = group[3]
            size = int(group[4], 16)
            path = group[5]
            # 我们将 .a 库下面的目标文件归成一类
            match = pattern1.search(line)
            if match:
                name = match.group(1)
                objname = match.group(2)
                if name in output_libs_sub.keys():
                    # libinfo = output_libs_sub[name]
                    if objname in output_libs_sub[name].keys():
                        # obj_info = output_libs_sub[name][objname]
                        if major in output_libs_sub[name][objname].keys():
                            output_libs_sub[name][objname][major] += size
                        else:
                            output_libs_sub[name][objname][major] = size
                    else:
                        output_libs_sub[name][objname] = {}
                        output_libs_sub[name][objname][major] = size
                else:
                    output_libs_sub[name] = {}
                    output_libs_sub[name][objname] = {}
                    output_libs_sub[name][objname][major] = size

            # 可能有重复的地址(多个变量共用一个地址)
            size = get_read_size(address, addr, group[4])
            total_size += size

            if major == 'text':
                get_read_size(text_address, addr, group[4])
                total_text += size

                if name in output_text.keys():
                    output_text[name] = output_text[name] + size
                    if name in output_dir.keys() and path == 'Null':
                        output_dir[name] += ', ' + minor
                    else:
                        output_dir[name] = path
                else:
                    output_text[name] = size
                    output_dir[name] = path

            if major == 'data':
                get_read_size(data_address, addr, group[4])
                total_data += size

                if name in output_data.keys():
                    output_data[name] = output_data[name] + size
                    if name in output_dir.keys() and path == 'Null':
                        output_dir[name] += ', ' + minor
                    else:
                        output_dir[name] = path
                else:
                    output_data[name] = size
                    output_dir[name] = path

            if major == 'bss':
                get_read_size(bss_address, addr, group[4])
                total_bss += size

                if name in output_bss.keys():
                    output_bss[name] = output_bss[name] + size
                    if name in output_dir.keys() and path == 'Null':
                        output_dir[name] += ', ' + minor
                    else:
                        output_dir[name] = path
                else:
                    output_bss[name] = size
                    if  path == 'Null':
                        output_dir[name] = minor
                    else:
                        output_dir[name] = path

            if name not in output_libs.keys():
                output_libs[name] = size
            else:
                output_libs[name] += size

    # sorrt this data
    sort_libs = sorted(output_libs.items(), key=lambda e:e[1], reverse=True)

    outfile = None
    if txt_file:
        outfile = open(txt_file, 'w')
    print_head(None)
    if outfile:
        print_head(outfile)
    print_line(None, total_size, "Total", total_size, total_data, total_text, total_bss, total_data, total_text, total_bss, '')
    if outfile:
        print_line(outfile, total_size, "Total", total_size, total_data, total_text, total_bss, total_data, total_text, total_bss, '')

    for item in sort_libs:
        name = item[0]
        file_size = item[1]

        if name in output_text.keys():
            file_text = output_text[name]
        else:
            file_text = 0

        if name in output_data.keys():
            file_data = output_data[name]
        else:
            file_data = 0

        if name in output_bss.keys():
            file_bss = output_bss[name]
        else:
            file_bss = 0

        path = output_dir[name]
        print_line(None, total_size, name, file_size, file_data, file_text, file_bss, total_data, total_text, total_bss, path)
        if outfile:
            print_line(outfile, total_size, name, file_size, file_data, file_text, file_bss, total_data, total_text, total_bss, path)
        match = pattern2.search(name)
        if match:
            for k2, v2 in output_libs_sub[name].items():
                # k2 = xxx.o
                # print("%s: text:%d data:%d bss:%s" %(k2, v2['text'], v2['data'], v2['bss']))
                text = 0
                bss = 0
                data = 0
                if 'text' in v2.keys():
                    text = v2['text']
                if 'bss' in v2.keys():
                    bss = v2['bss']
                if 'data'in v2.keys():
                    data = v2['data']
                print_subobj(None, k2, file_size, data, text, bss, file_data, file_text, file_bss)
                if outfile:
                    print_subobj(outfile, k2, file_size, data, text, bss, file_data, file_text, file_bss)

    print_line(None, total_size, "Total", total_size, total_data, total_text, total_bss, total_data, total_text, total_bss, '')
    if outfile:
        print_line(outfile, total_size, "Total", total_size, total_data, total_text, total_bss, total_data, total_text, total_bss, '')
        outfile.close()

    if debug == True:
        print('\n\n text aadress info:')
        for it in text_address:
            print('0x%x - 0x%x' % (it[0], it[1]))

        print('\n\n data aadress info:')
        for it in data_address:
            print('0x%x - 0x%x' % (it[0], it[1]))

        print('\n\n bss aadress info:')
        for it in bss_address:
            print('0x%x - 0x%x' % (it[0], it[1]))

def debug_save_line(out, minor, major, addr, size):
    line = "{: <50} {: <8} {: <20} {: <10}".format(
        minor, major, addr, size)
    out.write(line + '\n')  

# 生成debug文件(text.txt, data.txt, bss.txt, err.txt)
def gen_debug_file():
    text_pattern = re.compile('^(\S+) +(\.?\S+) +(0x\S+) +text +(0x[a-f0-9]+)')
    data_pattern = re.compile('^(\S+) +(\.?\S+) +(0x\S+) +data +(0x[a-f0-9]+)')
    bss_pattern = re.compile('^(\S+) +(\.?\S+) +(0x\S+) +bss +(0x[a-f0-9]+)')

    text_file = open("text.txt", 'w')
    data_file = open("data.txt", 'w')
    bss_file = open("bss.txt", 'w')
    err_file = open("err.txt", 'w')

    with open("out.txt", 'r') as f:
        for line in f:
            match1 = text_pattern.match(line)
            if match1:
                objname = match1.group(1)
                seg_name = match1.group(2)
                addr = match1.group(3)
                size = match1.group(4)
                debug_save_line(text_file, seg_name, 'text', addr, size)
                continue

            match2 = data_pattern.match(line)
            if match2:
                objname = match2.group(1)
                seg_name = match2.group(2)
                addr = match2.group(3)
                size = match2.group(4)
                debug_save_line(data_file, seg_name, 'data', addr, size)
                continue
            match3 = bss_pattern.match(line)
            if match3:
                objname = match3.group(1)
                seg_name = match3.group(2)
                addr = match3.group(3)
                size = match3.group(4)
                debug_save_line(bss_file, seg_name, 'bss', addr, size)
                continue
            err_file.write(line)

    text_file.close()
    data_file.close()
    bss_file.close()
    err_file.close()

print("cwd = %s" % cwd)

clean_tempfile()

update_segment_group()

# 生成 temp.txt, .temp.txt
find_segment(path,segment_start,segment_end)

# 生成 .out.txt
parser_target()

# 向 .out.txt 添加特殊处理的数据
nomatch = print_needed_segments()
find_needed_segment(nomatch)

# 根据 fill.txt 调整地址大小 并生成out.txt
adjust_targer_file()

# 根据 out.txt 输出最终数据
statistics_target()

if debug == True:
    # 生成调试信息文件 text.txt,bss.txt,data.txt
    gen_debug_file()
    print('Undown segment:')
    for item in undown_segment:
        print('\t ' + item)

if debug == True:
    print('')
    for k,v in elf_addr_range.items():
        print(k + ' address range:')
        size = 0
        for i in range(len(v)):
            size += v[i][1] - v[i][0]
            print('\t0x%x - 0x%x' % (v[i][0], v[i][1]))
        print('\tsize = %d(0x%x)\n' % (size, size))

if debug == False:
    clean_tempfile()
