#!/usr/bin/python3
# -*- coding:utf-8 -*-
# 设备端： 设备端执行：
#                 1.创建 adb 转发通路：af -p 20227 -r
#                 2.利用 adb_forward 接口发送数据

#         PC端：   1.创建adb转发通路：adb forward tcp:11112 tcp:20227
                                              #PC_port     dev_port
#                 2.开始接受数据
#
# 使用方法：1、python3 adb_record.py
#         2、python3 adb_record.py 11112    ---即带一个PC端口号


import datetime
import socket
import sys

pc_port = 11112
upload_file = "data.pcm"
upload_start_str = "-->AW_RTOS_SOCKET_UPLOAD_START"
upload_start = bytes(upload_start_str, encoding='utf-8')
upload_end_str = "-->AW_RTOS_SOCKET_UPLOAD_END"
upload_end = bytes(upload_end_str, encoding='utf-8')
data_length = 4096

def client_test(port):
    while True:
        print("start connect")
        s = socket.socket()
        host = "127.0.0.1"
        s.connect((host, port))
        while True:
            data = s.recv(data_length)
            if upload_start in data:
                print("recv upload start flag... data_len=%d, flag_len=%d" % (len(data), len(upload_start)))
                now_time = datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
                upload_file = "record-" + now_time + ".pcm"
                if len(upload_start) != len(data):
                    with open(upload_file, 'ab+') as f:
                        f.write(data[len(upload_start):])
            elif upload_end in data:
                index = data.find(upload_end)
                print("recv upload end flag...data_len=%d, str_index=%d" % (len(data), index))
                if index > 0:
                    with open(upload_file, 'ab+') as f:
                        f.write(data[:index])
                break
            else:
                if len(data) == 0:
                    print("data is null")
                    break
                with open(upload_file, 'ab+') as f:
                    f.write(data)
        print("recv finish!")
        s.close()

def main():
    if len(sys.argv) == 2:
        port = int(sys.argv[1])
    else:
        port = pc_port
    client_test(port)


if __name__ == '__main__':
    main()
