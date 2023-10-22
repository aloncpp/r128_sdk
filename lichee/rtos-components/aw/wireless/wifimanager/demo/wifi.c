#include <stdio.h>
#include <sys/types.h>
#if (OS_NET_LINUX_OS || OS_NET_XRLINK_OS)
#include <sys/socket.h>
#include <sys/un.h>
#endif
#include <unistd.h>
#include <string.h>
#ifdef OS_NET_FREERTOS_OS
#include "console.h"
#endif

#if (OS_NET_LINUX_OS || OS_NET_XRLINK_OS)
#define UNIX_DOMAIN "/tmp/UNIX_WIFI.domain"
#else
int rtos_main(void *snd_cmd);
#endif

#define SEND_BUF_SIZE    128

static void print_help()
{
#ifdef SUPPORT_STA_MODE
	printf("================sta mode Options===========\n"
		"wifi -o sta\n"
		"\t: open sta mode\n"
		"wifi -s\n"
		"\t: scan wifi\n"
		"wifi -c ssid [passwd]\n"
		"\t: connect to an encrypted or non-encrypted ap\n"
		"wifi -a [enable/disable]\n"
		"\t: Auto reconnect\n"
		"wifi -l [all]\n"
		"\t: list connected or saved ap information\n"
		"wifi -r [ssid/all]\n"
		"\t: remove a specified network or all networks\n"
		"wifi -d\n"
		"\t: disconnect from ap\n");
#endif
#ifdef SUPPORT_AP_MODE
	printf("================ap mode Options============\n"
		"wifi -o ap [ssid] [passwd] [channel]\n"
		"\t: open ap mode\n"
		"\t: if ssid and passwd is not set, start the default\n"
		"\t\tconfiguration:(allwinner-ap Aa123456 (channel:6))\n"
		"\t: if only set ssid, start the ap without passwd\n"
		"wifi -s ap\n"
		"\t: scan wifi (Only some systems perform scan in ap mode)\n"
		"wifi -l ap\n"
		"\t: list current ap mode information\n"
		"wifi -d ap\n"
		"\t: disable ap mode\n");
#endif
#ifdef SUPPORT_MONITOR_MODE
	printf("================monitor mode Options=======\n"
		"\t\t\tmonitor mode Options\n"
		"wifi -o monitor [channel]\n"
		"\t: open monitor mode\n"
		"\t: if channel is not set, start the default channel:6))\n"
		"wifi -d monitor\n"
		"\t: disable monitor mode\n");
#endif
#ifdef SUPPORT_P2P_MODE
	printf("================p2p mode Options===========\n"
		"wifi -o p2p [N:name] [I:intent] [T:time]\n"
		"\t: open p2p mode(with devname intent listen time)\n"
		"wifi -s p2p\n"
		"\t: find(scan) p2p dev\n"
		"wifi -C macaddr\n"
		"\t: connect to a p2p dev\n"
		"wifi -l p2p\n"
		"\t: list connected p2p dev(only in p2p go mode)\n"
		"wifi -d p2p\n"
		"\t: disable p2p mode\n");
#endif
	printf("================other Options==============\n"
		"wifi -D [error/warn/info/debug/dump/exce/add]\n"
		"\t: set debug level\n"
        "\t: open: print path file function line\n"
        "\t: close: do not print path file function line\n"
		"wifi -g\n"
		"\t: get system mac addr\n"
		"wifi -m [macaddr]\n"
		"\t: set system mac addr\n"
		"wifi -i\n"
		"\t: get wifimanager info\n");
#if OS_NET_XRLINK_OS
	printf("wifi -v [length]\n"
		"\t: wifi user vendor msg test\n");
#endif
#ifdef SUPPORT_EXPAND
	printf("wifi -e [linux: XXX]/[xrlink: XXX]/[freertos: XXX]\n"
		"\t: wifi send expand cmd\n");
#endif
#ifdef SUPPORT_LINKD
	printf("wifi -p [softap/ble/xconfig/soundwave]\n"
		"\t: softap/ble/xconfig/soundwave distribution network\n");
#endif
	printf("wifi -f\n"
		"\t: close wifimanager\n"
		"wifi -h\n"
		"\t: print help\n"
		"==============================================\n");
}

int main(int argc, char *argv[])
{
	int connect_fd;
	int ret;
	char snd_buf[SEND_BUF_SIZE];
#if (OS_NET_LINUX_OS || OS_NET_XRLINK_OS)
	static struct sockaddr_un srv_addr;
#endif

	if((argc > 6 || argc <= 1) || (strncmp(argv[1], "-", 1)) || (strlen(argv[1]) != 2)){
		print_help();
		return -1;
	}

	memset(snd_buf, 0, SEND_BUF_SIZE);

	int ch = 0;
	for (;;) {
		ch = getopt(argc, argv, "ac:C:dfgil::m:o:p:r:sv:e:D:h");
		if (ch < 0)
			break;
		switch (ch) {
			case 'o':
				if(!strcmp(argv[2], "sta")) {
					if(argc != 3) {
						print_help();
						return -1;
					}
					sprintf(snd_buf, "o sta");
#ifdef SUPPORT_AP_MODE
				} else if (!strcmp(argv[2], "ap")) {
					if(argc == 4) {
						sprintf(snd_buf, "o ap %s",argv[3]);
					} else if(argc == 5) {
						sprintf(snd_buf, "o ap %s %s",argv[3], argv[4]);
					} else if(argc == 6) {
						sprintf(snd_buf, "o ap %s %s %s",argv[3], argv[4], argv[5]);
					} else {
						sprintf(snd_buf, "o ap");
					}
#endif
#ifdef SUPPORT_MONITOR_MODE
				} else if(!strcmp(argv[2], "monitor")) {
					if(argc == 3) {
						sprintf(snd_buf, "o monitor");
					} else if(argc == 4){
						sprintf(snd_buf, "o monitor %s",argv[3]);
					} else {
						print_help();
						return -1;
					}
#endif
#ifdef SUPPORT_P2P_MODE
				} else if(!strcmp(argv[2], "p2p")) {
					if(argc == 3) {
						sprintf(snd_buf, "o p2p");
					} else if(argc == 4) {
						sprintf(snd_buf, "o p2p %s",argv[3]);
					} else if(argc == 5) {
						sprintf(snd_buf, "o p2p %s %s",argv[3], argv[4]);
					} else if(argc == 6) {
						sprintf(snd_buf, "o p2p %s %s %s",argv[3], argv[4], argv[5]);
					} else {
						print_help();
						return -1;
					}
#endif
				} else {
					print_help();
					return -1;
				}
				goto send_cmd;
			case 'f':
				if(argc > 2) {
					print_help();
					return -1;
				}
				sprintf(snd_buf, "f");
				goto send_cmd;
			case 's':
				if(argc == 3) {
					sprintf(snd_buf, "s %s",argv[2]);
				} else if (argc > 3){
					print_help();
					return -1;
				} else {
					sprintf(snd_buf, "s");
				}
				goto send_cmd;
#ifdef SUPPORT_STA_MODE
			case 'c':
				if(argc == 3) {
					sprintf(snd_buf, "c %s",argv[2]);
				} else if (argc == 4) {
					sprintf(snd_buf, "c %s %s",argv[2], argv[3]);
				} else if (argc == 5) {
					sprintf(snd_buf, "c %s %s %s",argv[2], argv[3], argv[4]);
				} else {
					print_help();
					return -1;
				}
				goto send_cmd;
#endif
#ifdef SUPPORT_P2P_MODE
			case 'C':
				if(argc == 3) {
					sprintf(snd_buf, "C %s",argv[2]);
				} else {
					print_help();
					return -1;
				}
				goto send_cmd;
#endif
			case 'd':
				if(argc > 3) {
					print_help();
					return -1;
				} else if (argc == 2) {
					sprintf(snd_buf, "d");
				} else {
					sprintf(snd_buf, "d %s",argv[2]);
				}
				goto send_cmd;
			case 'a':
				if(argc == 3) {
					sprintf(snd_buf, "a %s", argv[2]);
				} else {
					print_help();
					return -1;
				}
				goto send_cmd;
			case 'l':
				if(argc == 2) {
					sprintf(snd_buf, "l");
				} else if (argc == 3) {
					if(!strcmp(argv[2], "all")) {
						sprintf(snd_buf, "l all");
					} else if(!strcmp(argv[2], "ap")) {
						sprintf(snd_buf, "l ap");
					} else if(!strcmp(argv[2], "p2p")) {
						sprintf(snd_buf, "l p2p");
					} else {
						print_help();
						return -1;
					}
				}
				goto send_cmd;
			case 'r':
				if(argc == 3) {
					if(!strcmp(argv[2], "all")) {
						sprintf(snd_buf, "r all");
					} else {
						sprintf(snd_buf, "r %s", argv[2]);
					}
				} else {
					print_help();
					return -1;
				}
				goto send_cmd;
			case 'g':
				if(argc > 2) {
					print_help();
					return -1;
				}
				sprintf(snd_buf, "g");
				goto send_cmd;
			case 'm':
				if(argc == 3) {
					sprintf(snd_buf, "m %s", argv[2]);
				} else {
					print_help();
					return -1;
				}
				goto send_cmd;
			case 'i':
				if(argc > 2) {
					print_help();
					return -1;
				}
				sprintf(snd_buf, "i");
				goto send_cmd;
#if OS_NET_XRLINK_OS
			case 'v':
				if(argc == 3) {
					sprintf(snd_buf, "v %s", argv[2]);
				} else {
					print_help();
					return -1;
				}
				goto send_cmd;
#endif
#ifdef SUPPORT_EXPAND
			case 'e':
				if(argc == 3) {
					sprintf(snd_buf, "e %s", argv[2]);
				} else {
					print_help();
					return -1;
				}
				goto send_cmd;
#endif
#ifdef SUPPORT_LINKD
			case 'p':
				if(argc == 3) {
					sprintf(snd_buf, "p %s", argv[2]);
				} else {
					print_help();
					return -1;
				}
				goto send_cmd;
#endif
			case 'D':
				if(argc == 3) {
					sprintf(snd_buf, "D %s", argv[2]);
				} else {
					print_help();
					return -1;
				}
				goto send_cmd;
			case '?':
				printf("Unknown option: %c\n",(char)optopt);
			case 'h':
				print_help();
				return -1;
		}
	}

send_cmd:
#if (OS_NET_LINUX_OS || OS_NET_XRLINK_OS)
	//create unix socket
	connect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(connect_fd < 0) {
		perror("Can't create wifi deamon communication socket\n");
		return -1;
	}
	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path,UNIX_DOMAIN);

	//connect server
	ret = connect(connect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if(ret == -1) {
		perror("Can't connect to the wifi daemon server");
		close(connect_fd);
		return ret;
	}

	write(connect_fd, snd_buf, sizeof(snd_buf));
	close(connect_fd);
#else
	rtos_main(snd_buf);
#endif //OS_NET_LINUX_OS
	return 0;
}

#ifdef OS_NET_FREERTOS_OS
FINSH_FUNCTION_EXPORT_CMD(main, wifi, wifi testcmd);
#endif
