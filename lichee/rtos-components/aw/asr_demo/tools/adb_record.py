#!/usr/bin/python2

import sys
import socket
import time
import datetime

pc_port=11112
upload_file="data.pcm"
upload_start="-->AW_RTOS_SOCKET_UPLOAD_START"
upload_end="-->AW_RTOS_SOCKET_UPLOAD_END"
data_length=4096

def client_test(port):
    while True:
        print 'start connect'
        s = socket.socket()
        host = "localhost"
        s.connect((host,port))
        while True:
            data = s.recv(data_length)
            if (len(data) != 0):
                print 'recv %d' % (len(data))
            if upload_start in data:
                print 'recv upload start flag...data_len=%d, flag_len=%d' % (len(data), len(upload_start))
                now_time = datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
                upload_file="record-" + now_time + ".pcm"
                if (len(upload_start) != len(data)):
                    with open(upload_file.decode('utf-8'), 'ab+') as f:
                        f.write(data[len(upload_start):])
            elif upload_end in data:
                index = data.find(upload_end)
                print 'recv upload end flag...data_len=%d, str_index=%d' % (len(data), index)
                if index > 0:
                    with open(upload_file.decode('utf-8'), 'ab+') as f:
                        f.write(data[:index])
                break
            else:
                if (len(data) == 0):
                    print 'data is 0'
                    break;
                # print 'recv data, len is ', len(data)
                with open(upload_file.decode('utf-8'), 'ab+') as f:
                    f.write(data)
        print 'finish...'
        s.close()

def main():
    if (len(sys.argv) == 2):
        port = int(sys.argv[1])
    else:
        port = pc_port
    client_test(port)

if __name__=='__main__':
	main()
