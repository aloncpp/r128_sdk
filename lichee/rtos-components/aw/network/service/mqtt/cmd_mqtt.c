/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cmd_util.h"
#include "MQTTClient.h"
#include <console.h>

#define MQTT_BUF_SIZE (1024)

#if defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_ARCH_RISCV_RV64)
#define MQTT_CLI_TASK_STACK_SIZE (12 * 1024)
#else
#define MQTT_CLI_TASK_STACK_SIZE (6 * 1024)
#endif

typedef enum {
	MQTT_CMD_CONNECT,
	MQTT_CMD_DISCONNECT,
	MQTT_CMD_SUBSCRIBE,
	MQTT_CMD_UNSUBSCRIBE,
	MQTT_CMD_PUBLISH,
	MQTT_CMD_EXIT
} MQTT_CMD;

struct mqtt_msg {
	MQTT_CMD type;
	void *data;
	int data_len;
};

struct publish_info {
	int qos;
	int retain;
	char topic[128];
	char message[128];
};

struct subscribe_info {
	int qos;
	char topic[128];
};

struct unsubscribe_info {
	char topic[128];
};

struct cmd_mqtt_common {
	int ssl;
	int alive;
	int clean;
	int will_qos;
	int will_retain;
	char *will_topic;
	char *will_message;
	char *username;
	char *password;
	char port[8];
	char host[128];
	char client_id[128];
	char *sub_topic[MAX_MESSAGE_HANDLERS];
	XR_OS_Thread_t thread;
	XR_OS_Queue_t queue;
};

static struct cmd_mqtt_common cmd_mqtt;

static void mqtt_msg_cb(MessageData* data)
{
	CMD_DBG("[topic: %.*s] %.*s\n", data->topicName->lenstring.len,
	          data->topicName->lenstring.data, data->message->payloadlen,
	          (char *)data->message->payload);
}

static int mqtt_cmd_connect_handler(Client *client, Network *network, void *data, int data_len)
{
	int ret;
	unsigned char *send_buf = NULL;
	unsigned char *recv_buf = NULL;
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

	send_buf = cmd_malloc(MQTT_BUF_SIZE);
	if (send_buf == NULL) {
		CMD_ERR("malloc fail.\n");
		return -1;
	}
	recv_buf = cmd_malloc(MQTT_BUF_SIZE);
	if (recv_buf == NULL) {
		CMD_ERR("malloc fail.\n");
		goto err;
	}

	NewNetwork(network);
	MQTTClient(client, network, 6000, send_buf, MQTT_BUF_SIZE, recv_buf, MQTT_BUF_SIZE);

	if (cmd_mqtt.ssl) {
		/* this test has no ca cert, no client cert, no client pk, no client pwd */
		ret = TLSConnectNetwork(network, cmd_mqtt.host, cmd_mqtt.port, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
		if (ret != 0) {
			CMD_ERR("Return code from network ssl connect is -0x%04x\n", -ret);
			goto err;
		}
	} else {
		ret = ConnectNetwork(network, cmd_mqtt.host, atoi(cmd_mqtt.port));
		if (ret != 0) {
			CMD_ERR("Return code from network connect is %d\n", ret);
			goto err;
		}
	}

	connectData.clientID.cstring = cmd_mqtt.client_id;
	connectData.keepAliveInterval = cmd_mqtt.alive;
	connectData.cleansession = cmd_mqtt.clean;
	connectData.MQTTVersion = 4; //Version of MQTT 3.1.1

	if (cmd_mqtt.will_topic && cmd_mqtt.will_message) {
		connectData.willFlag = 1;
		connectData.will.topicName.cstring = cmd_mqtt.will_topic;
		connectData.will.message.cstring = cmd_mqtt.will_message;
		connectData.will.retained = cmd_mqtt.will_retain;
		connectData.will.qos = cmd_mqtt.will_qos;
	}
	if (cmd_mqtt.username) {
		connectData.username.cstring = cmd_mqtt.username;
	}
	if (cmd_mqtt.password) {
		connectData.password.cstring = cmd_mqtt.password;
	}
	ret = MQTTConnect(client, &connectData);
	if (ret != 0) {
		CMD_ERR("Return code from MQTT connect is %d\n", ret);
		network->disconnect(network);
		goto err;
	}

	CMD_DBG("MQTT connect success.\n");
	return 0;

err:
	cmd_free(send_buf);
	cmd_free(recv_buf);
	return -1;
}

static int mqtt_cmd_disconnect_handler(Client *client, Network *network, void *data, int data_len)
{
	int i;
	int ret;

	ret = MQTTDisconnect(client);
	if (ret != 0) {
		CMD_ERR("Return code from MQTT disconnect is %d\n", ret);
		network->disconnect(network);
		return -1;
	}
	network->disconnect(network);
	cmd_free(client->buf);
	cmd_free(client->readbuf);
	for (i = 0; i < MAX_MESSAGE_HANDLERS; i++) {
		cmd_free(cmd_mqtt.sub_topic[i]);
		cmd_mqtt.sub_topic[i] = NULL;
	}
	CMD_DBG("MQTT disconnect success.\n");
	return 0;
}

static int mqtt_cmd_subscribe_handler(Client *client, Network *network, void *data, int data_len)
{
	int i;
	int ret;
	int topic_len;
	struct subscribe_info *info = data;

	topic_len = cmd_strlen(info->topic) + 1;
	for (i = 0; i < MAX_MESSAGE_HANDLERS; i++) {
		if (cmd_mqtt.sub_topic[i] == NULL) {
			cmd_mqtt.sub_topic[i] = malloc(topic_len);
			if (cmd_mqtt.sub_topic[i] == NULL) {
				return -1;
			}
			cmd_memcpy(cmd_mqtt.sub_topic[i], info->topic, topic_len);
			break;
		}
	}
	if (i >= MAX_MESSAGE_HANDLERS) {
		CMD_ERR("Subscribe topic limit %d\n", MAX_MESSAGE_HANDLERS);
		return -1;
	}

	ret = MQTTSubscribe(client, cmd_mqtt.sub_topic[i], info->qos, mqtt_msg_cb);
	if (ret != 0) {
		CMD_ERR("Return code from MQTT subscribe is %d\n", ret);
		cmd_free(cmd_mqtt.sub_topic[i]);
		cmd_mqtt.sub_topic[i] = NULL;
		return -1;
	}
	CMD_DBG("MQTT subscribe success.\n");
	return 0;
}

static int mqtt_cmd_unsubscribe_handler(Client *client, Network *network, void *data, int data_len)
{
	int i;
	int ret;
	struct unsubscribe_info *info = data;

	ret = MQTTUnsubscribe(client, info->topic);
	if (ret != 0) {
		CMD_ERR("Return code from MQTT unsubscribe is %d\n", ret);
		return -1;
	}

	for (i = 0; i < MAX_MESSAGE_HANDLERS; i++) {
		if (cmd_mqtt.sub_topic[i] != NULL && !cmd_strncmp(cmd_mqtt.sub_topic[i], info->topic, cmd_strlen(info->topic))) {
			cmd_free(cmd_mqtt.sub_topic[i]);
			cmd_mqtt.sub_topic[i] = NULL;
			break;
		}
	}
	if (i >= MAX_MESSAGE_HANDLERS) {
		CMD_ERR("MQTT unsubscribe fail.No such topic\n");
		return -1;
	}
	CMD_DBG("MQTT unsubscribe success.\n");
	return 0;
}

static int mqtt_cmd_publish_handler(Client *client, Network *network, void *data, int data_len)
{
	int ret;
	MQTTMessage message;
	struct publish_info *info = data;

	message.qos = info->qos;
	message.retained = info->retain;
	message.payload = info->message;
	message.payloadlen = cmd_strlen(info->message);

	ret = MQTTPublish(client, info->topic, &message);
	if (ret != 0) {
		CMD_ERR("Return code from MQTT publish is %d\n", ret);
		return -1;
	}

	CMD_DBG("MQTT publish success.\n");
	return 0;
}

static void mqtt_client_task(void *arg)
{
	int exit = 0;
	int connected = 0;
	XR_OS_Status status;
	struct mqtt_msg *msg;
	Client client;
	Network network;

	while (!exit) {
		status = XR_OS_MsgQueueReceive(&cmd_mqtt.queue, (void **)&msg, 0);
		if (status != XR_OS_OK) {
			if (connected) {
				MQTTYield(&client, 3000);
			}
			XR_OS_MSleep(50);
			continue;
		}

		CMD_DBG("msg type:%d\n", msg->type);
		switch (msg->type) {
		case MQTT_CMD_CONNECT:
			cmd_memset(&client, 0, sizeof(Client));
			cmd_memset(&network, 0, sizeof(Network));
			mqtt_cmd_connect_handler(&client, &network, msg->data, msg->data_len);
			connected = 1;
			break;
		case MQTT_CMD_DISCONNECT:
			mqtt_cmd_disconnect_handler(&client, &network, msg->data, msg->data_len);
			connected = 0;
			break;
		case MQTT_CMD_SUBSCRIBE:
			mqtt_cmd_subscribe_handler(&client, &network, msg->data, msg->data_len);
			break;
		case MQTT_CMD_UNSUBSCRIBE:
			mqtt_cmd_unsubscribe_handler(&client, &network, msg->data, msg->data_len);
			break;
		case MQTT_CMD_PUBLISH:
			mqtt_cmd_publish_handler(&client, &network, msg->data, msg->data_len);
			break;
		case MQTT_CMD_EXIT:
			exit = 1;
			break;
		default:
			break;
		}
		cmd_free(msg->data);
		cmd_free(msg);
	}
	XR_OS_ThreadDelete(&cmd_mqtt.thread);
}

static enum cmd_status cmd_mqtt_init_exec(char *cmd)
{
	if (XR_OS_MsgQueueCreate(&cmd_mqtt.queue, 4) != XR_OS_OK) {
		CMD_ERR("create queue failed\n");
		return CMD_STATUS_FAIL;
	}

	if (XR_OS_ThreadCreate(&cmd_mqtt.thread,
	                    "mqtt client",
	                    mqtt_client_task,
	                    NULL,
	                    XR_OS_THREAD_PRIO_APP,
	                    MQTT_CLI_TASK_STACK_SIZE) != XR_OS_OK) {
		CMD_ERR("create thread failed\n");
		XR_OS_MsgQueueDelete(&cmd_mqtt.queue);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_mqtt_config_exec(char *cmd)
{
	int i;
	int len;
	int argc;
	char *argv[20];

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	for (i = 0; i < argc; i++) {
		if (cmd_strstr(argv[i], "host=")) {
			cmd_strlcpy(cmd_mqtt.host, argv[i] + 5, sizeof(cmd_mqtt.host));
		} else if (cmd_strstr(argv[i], "port=")) {
			cmd_strlcpy(cmd_mqtt.port, argv[i] + 5, sizeof(cmd_mqtt.port));
		} else if (cmd_strstr(argv[i], "client_id=")) {
			cmd_strlcpy(cmd_mqtt.client_id, argv[i] + 10, sizeof(cmd_mqtt.client_id));
		} else if (cmd_strstr(argv[i], "username=")) {
			len = cmd_strlen(argv[i]);
			cmd_mqtt.username = cmd_malloc(len);
			cmd_strlcpy(cmd_mqtt.username, argv[i] + 9, len);
		} else if (cmd_strstr(argv[i], "password=")) {
			len = cmd_strlen(argv[i]);
			cmd_mqtt.password = cmd_malloc(len);
			cmd_strlcpy(cmd_mqtt.password, argv[i] + 9, len);
		} else if (cmd_strstr(argv[i], "alive=")) {
			cmd_sscanf(argv[i], "alive=%d", &cmd_mqtt.alive);
		} else if (cmd_strstr(argv[i], "ssl=")) {
			cmd_sscanf(argv[i], "ssl=%d", &cmd_mqtt.ssl);
		} else if (cmd_strstr(argv[i], "clean=")) {
			cmd_sscanf(argv[i], "clean=%d", &cmd_mqtt.clean);
		} else if (cmd_strstr(argv[i], "will_topic=")) {
			len = cmd_strlen(argv[i]);
			cmd_mqtt.will_topic = cmd_malloc(len);
			cmd_strlcpy(cmd_mqtt.will_topic, argv[i] + 11, len);
		} else if (cmd_strstr(argv[i], "will_message=")) {
			len = cmd_strlen(argv[i]);
			cmd_mqtt.will_message = cmd_malloc(len);
			cmd_strlcpy(cmd_mqtt.will_message, argv[i] + 13, len);
		} else if (cmd_strstr(argv[i], "will_qos=")) {
			cmd_sscanf(argv[i], "will_qos=%d", &cmd_mqtt.will_qos);
		} else if (cmd_strstr(argv[i], "will_retain=")) {
			cmd_sscanf(argv[i], "will_retain=%d", &cmd_mqtt.will_retain);
		} else {
			CMD_ERR("invalid param:%s\n", argv[i]);
		}
	}
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_mqtt_connect_exec(char *cmd)
{
	XR_OS_Status status;
	struct mqtt_msg *msg;

	msg = cmd_malloc(sizeof(struct mqtt_msg));
	if (msg == NULL) {
		return CMD_STATUS_FAIL;
	}
	cmd_memset(msg, 0, sizeof(struct mqtt_msg));
	msg->type = MQTT_CMD_CONNECT;
	msg->data = NULL;
	msg->data_len = 0;

	status = XR_OS_MsgQueueSend(&cmd_mqtt.queue, msg, XR_OS_WAIT_FOREVER);
	if (status != XR_OS_OK) {
		cmd_free(msg);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_mqtt_disconnect_exec(char *cmd)
{
	XR_OS_Status status;
	struct mqtt_msg *msg;

	msg = cmd_malloc(sizeof(struct mqtt_msg));
	if (msg == NULL) {
		return CMD_STATUS_FAIL;
	}
	cmd_memset(msg, 0, sizeof(struct mqtt_msg));
	msg->type = MQTT_CMD_DISCONNECT;
	msg->data = NULL;
	msg->data_len = 0;

	status = XR_OS_MsgQueueSend(&cmd_mqtt.queue, msg, XR_OS_WAIT_FOREVER);
	if (status != XR_OS_OK) {
		cmd_free(msg);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_mqtt_subscribe_exec(char *cmd)
{
	int cnt;
	XR_OS_Status status;
	struct mqtt_msg *msg;
	struct subscribe_info *info;

	info = cmd_malloc(sizeof(struct subscribe_info));
	if (info == NULL) {
		return CMD_STATUS_FAIL;
	}
	cmd_memset(info, 0, sizeof(struct subscribe_info));
	cnt = cmd_sscanf(cmd, "qos=%u topic=%128s", &info->qos, info->topic);
	if (cnt != 2) {
		CMD_ERR("invalid param number %d\n", cnt);
		cmd_free(info);
		return CMD_STATUS_INVALID_ARG;
	}

	msg = cmd_malloc(sizeof(struct mqtt_msg));
	if (msg == NULL) {
		cmd_free(info);
		return CMD_STATUS_FAIL;
	}
	cmd_memset(msg, 0, sizeof(struct mqtt_msg));
	msg->type = MQTT_CMD_SUBSCRIBE;
	msg->data = info;
	msg->data_len = sizeof(struct subscribe_info);

	status = XR_OS_MsgQueueSend(&cmd_mqtt.queue, msg, XR_OS_WAIT_FOREVER);
	if (status != XR_OS_OK) {
		cmd_free(info);
		cmd_free(msg);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_mqtt_unsubscribe_exec(char *cmd)
{
	int cnt;
	XR_OS_Status status;
	struct mqtt_msg *msg;
	struct unsubscribe_info *info;

	info = cmd_malloc(sizeof(struct unsubscribe_info));
	if (info == NULL) {
		return CMD_STATUS_FAIL;
	}
	cmd_memset(info, 0, sizeof(struct unsubscribe_info));
	cnt = cmd_sscanf(cmd, "topic=%128s",info->topic);
	if (cnt != 1) {
		CMD_ERR("invalid param number %d\n", cnt);
		cmd_free(info);
		return CMD_STATUS_INVALID_ARG;
	}

	msg = cmd_malloc(sizeof(struct mqtt_msg));
	if (msg == NULL) {
		cmd_free(info);
		return CMD_STATUS_FAIL;
	}
	cmd_memset(msg, 0, sizeof(struct mqtt_msg));
	msg->type = MQTT_CMD_UNSUBSCRIBE;
	msg->data = info;
	msg->data_len = sizeof(struct unsubscribe_info);

	status = XR_OS_MsgQueueSend(&cmd_mqtt.queue, msg, XR_OS_WAIT_FOREVER);
	if (status != XR_OS_OK) {
		cmd_free(info);
		cmd_free(msg);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_mqtt_publish_exec(char *cmd)
{
	int cnt;
	XR_OS_Status status;
	struct mqtt_msg *msg;
	struct publish_info *info;

	info = cmd_malloc(sizeof(struct publish_info));
	if (info == NULL) {
		return CMD_STATUS_FAIL;
	}
	cmd_memset(info, 0, sizeof(struct publish_info));
	cnt = cmd_sscanf(cmd, "qos=%d retain=%d topic=%128s message=%128s",
	              &info->qos, &info->retain, info->topic, info->message);
	if (cnt != 4) {
		CMD_ERR("invalid param number %d\n", cnt);
		cmd_free(info);
		return CMD_STATUS_INVALID_ARG;
	}

	msg = cmd_malloc(sizeof(struct mqtt_msg));
	if (msg == NULL) {
		cmd_free(info);
		return CMD_STATUS_FAIL;
	}
	cmd_memset(msg, 0, sizeof(struct mqtt_msg));
	msg->type = MQTT_CMD_PUBLISH;
	msg->data = info;
	msg->data_len = sizeof(struct publish_info);

	status = XR_OS_MsgQueueSend(&cmd_mqtt.queue, msg, XR_OS_WAIT_FOREVER);
	if (status != XR_OS_OK) {
		cmd_free(info);
		cmd_free(msg);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_mqtt_deinit_exec(char *cmd)
{
	XR_OS_Status status;
	struct mqtt_msg *msg;

	msg = cmd_malloc(sizeof(struct mqtt_msg));
	if (msg == NULL) {
		return CMD_STATUS_FAIL;
	}
	cmd_memset(msg, 0, sizeof(struct mqtt_msg));
	msg->type = MQTT_CMD_EXIT;
	msg->data = NULL;
	msg->data_len = 0;

	status = XR_OS_MsgQueueSend(&cmd_mqtt.queue, msg, XR_OS_WAIT_FOREVER);
	if (status != XR_OS_OK) {
		cmd_free(msg);
		return CMD_STATUS_FAIL;
	}
	while (XR_OS_ThreadIsValid(&cmd_mqtt.thread)) {
		XR_OS_MSleep(10);
	}
	XR_OS_MsgQueueDelete(&cmd_mqtt.queue);
	cmd_free(cmd_mqtt.will_topic);
	cmd_free(cmd_mqtt.will_message);
	cmd_free(cmd_mqtt.username);
	cmd_free(cmd_mqtt.password);
	cmd_memset(&cmd_mqtt, 0, sizeof(struct cmd_mqtt_common));

	return CMD_STATUS_OK;
}

#if CMD_DESCRIBE
#define mqtt_init_help_info "init"
#define mqtt_config_help_info "config <field>=<value>..."
#define mqtt_connect_help_info "connect to server"
#define mqtt_subscribe_help_info "subscribe qos=<qos> topic=<topic>"
#define mqtt_unsubscribe_help_info "unsubscribe topic=<topic>"
#define mqtt_publish_help_info "publish qos=<qos> retain=<retain> topic=<topic> message=<message>"
#define mqtt_disconnect_help_info "disconnect from server"
#define mqtt_deinit_help_info "deinit"
#endif

enum cmd_status cmd_mqtt_help_exec(char *cmd);

static const struct cmd_data g_mqtt_cmds[] = {
	{ "init",        cmd_mqtt_init_exec, CMD_DESC(mqtt_init_help_info) },
	{ "config",      cmd_mqtt_config_exec, CMD_DESC(mqtt_config_help_info) },
	{ "connect",     cmd_mqtt_connect_exec, CMD_DESC(mqtt_connect_help_info) },
	{ "subscribe",   cmd_mqtt_subscribe_exec, CMD_DESC(mqtt_subscribe_help_info) },
	{ "unsubscribe", cmd_mqtt_unsubscribe_exec, CMD_DESC(mqtt_unsubscribe_help_info) },
	{ "publish",     cmd_mqtt_publish_exec, CMD_DESC(mqtt_publish_help_info) },
	{ "disconnect",  cmd_mqtt_disconnect_exec, CMD_DESC(mqtt_disconnect_help_info) },
	{ "deinit",      cmd_mqtt_deinit_exec, CMD_DESC(mqtt_deinit_help_info) },
	{ "help",        cmd_mqtt_help_exec, CMD_DESC(CMD_HELP_DESC) },
};

enum cmd_status cmd_mqtt_help_exec(char *cmd)
{
	return cmd_help_exec(g_mqtt_cmds, cmd_nitems(g_mqtt_cmds), 16);
}

enum cmd_status cmd_mqtt_exec(char *cmd)
{
	return cmd_exec(cmd, g_mqtt_cmds, cmd_nitems(g_mqtt_cmds));
}

static void mqtt_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_mqtt_exec);
}

FINSH_FUNCTION_EXPORT_CMD(mqtt_exec, mqtt, mqtt testcmd);
