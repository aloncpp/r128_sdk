/******************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tinatest.h"
#include "xml_packet.h"

//#define ARRAY_SIZE(arr) ( sizeof(arr)/sizeof((arr)[0]) )

#define LABEL_VAL_LEN_MAX		256//>"msgid", "cmd", "name", testcase name lenth

/*********************************************************************
* name: buildXmlNodeInfoNew
* function: build xml node information
* input: nodeName - node name, nodeValue - node value
* return: baseNode - xml node information
*********************************************************************/
int buildXmlNodeInfoNew(const char *nodeName[], const char nodeValue[][LABEL_VAL_LEN_MAX], struct xmlNode *baseNode)
{
	int i;
	for(i = 0; nodeName[i] != NULL; i++)
	{
		strcpy(baseNode[i].name, nodeName[i]);
		strcpy(baseNode[i].content, nodeValue[i]);
	}
	baseNode[i].name[0] = '\0';
	baseNode[i].content[0] = '\0';

	return 0;
}

/*********************************************************************
 * name: fill_packet_start
 * input:
 * testname: testcase name
 * id: msg id
 * limit: max lenth of packet
 * input && output:
 * buf: buffer of packet
 * return: lenth of packet
*********************************************************************/
int fill_packet_start(char *buf, int limit, const char *testname, int id){
	if(!testname || !buf)
		return -1;
	
	const char *label[] = {"msgid", "cmd", "name", NULL};

	char label_val[3][LABEL_VAL_LEN_MAX];
	snprintf(label_val[0], LABEL_VAL_LEN_MAX-1, "%d", id);
	strncpy(label_val[1], "start", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[2], testname, LABEL_VAL_LEN_MAX-1);

	struct xmlNode baseNode[4];
	buildXmlNodeInfoNew(label, label_val, baseNode);

	int len = 1024;
	if(limit<len){
		ERROR("limit(%d)<len(%d)\n", limit, len);
		return -1;
	}
	memset(buf, 0, limit);

	CreateXmlPacket("req", baseNode, ARRAY_SIZE(baseNode), buf);

	return strlen(buf);
}

/*********************************************************************
 * name: fill_packet_result
 * input:
 * testname: testcase name
 * id: msg id
 * limit: max lenth of packet
 * result: 1(ok)/other(FAIL)
 * mark: mark
 * input && output:
 * buf: buffer of packet
 * return: lenth of packet
*********************************************************************/
int fill_packet_end(char *buf, int limit, const char *testname, int id, int result, const char *mark){
	if(!testname || !buf || !mark)
		return -1;

	const char *label[] = {"msgid", "cmd", "name", "result", "mark", NULL};
	char label_val[5][LABEL_VAL_LEN_MAX];
	snprintf(label_val[0], LABEL_VAL_LEN_MAX-1, "%d", id);
	strncpy(label_val[1], "end", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[2], testname, LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[3], (result==1)?"OK":"FAIL", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[4], mark, LABEL_VAL_LEN_MAX-1);

	struct xmlNode baseNode[6];
	buildXmlNodeInfoNew(label, label_val, baseNode);

	int len = 1024;
	if(limit<len){
		ERROR("limit(%d)<len(%d)\n", limit, len);
		return -1;
	}
	memset(buf, 0, limit);

	CreateXmlPacket("req", baseNode, ARRAY_SIZE(baseNode), buf);

	return strlen(buf);
}

/*********************************************************************
 * name: fill_packet_finish
 * input:
 * id: msg id
 * limit: max lenth of packet
 * result: 1(ok)/other(FAIL)
 * mark: mark
 * input && output:
 * buf: buffer of packet
 * return: lenth of packet
*********************************************************************/
int fill_packet_finish(char *buf, int limit, int id, int result, const char *mark){
	if(!buf || !mark)
		return -1;
	
	const char *label[] = {"msgid", "cmd", "name", "result", "mark", NULL};

	char label_val[5][LABEL_VAL_LEN_MAX];
	snprintf(label_val[0], LABEL_VAL_LEN_MAX-1, "%d", id);
	strncpy(label_val[1], "finish", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[2], "null", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[3], (result==1)?"OK":"FAIL", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[4], mark, LABEL_VAL_LEN_MAX-1);

	struct xmlNode baseNode[6];
	buildXmlNodeInfoNew(label, label_val, baseNode);

	int len = 1024;
	if(limit<len){
		ERROR("limit(%d)<len(%d)\n", limit, len);
		return -1;
	}
	memset(buf, 0, limit);

	CreateXmlPacket("req", baseNode, ARRAY_SIZE(baseNode), buf);

	return strlen(buf);
}

/*********************************************************************
 * name: fill_packet_tip
 * input:
 * testname: testcase name
 * id: msg id
 * limit: max lenth of packet
 * tip: tip to show
 * input && output:
 * buf: buffer of packet
 * return: lenth of packet
*********************************************************************/
int fill_packet_tip(char *buf, int limit, const char *testname, int id, const char *tip){
	if(!testname || !buf || !tip)
		return -1;
	
	const char *label[] = {"msgid", "cmd", "name", "tip", NULL};

	char label_val[4][LABEL_VAL_LEN_MAX];
	snprintf(label_val[0], LABEL_VAL_LEN_MAX-1, "%d", id);
	strncpy(label_val[1], "tip", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[2], testname, LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[3], tip, LABEL_VAL_LEN_MAX-1);

	struct xmlNode baseNode[5];
	buildXmlNodeInfoNew(label, label_val, baseNode);

	int len = 1024;
	if(limit<len){
		ERROR("limit(%d)<len(%d)\n", limit, len);
		return -1;
	}
	memset(buf, 0, limit);

	CreateXmlPacket("req", baseNode, ARRAY_SIZE(baseNode), buf);

	return strlen(buf);
}

/*********************************************************************
 * name: fill_packet_select
 * input:
 * testname: testcase name
 * id: msg id
 * limit: max lenth of packet
 * tip: tip to show
 * mark: mark
 * timeout: timeout
 * input && output:
 * buf: buffer of packet
 * return: lenth of packet
*********************************************************************/
int fill_packet_select(char *buf, int limit, const char *testname, int id, const char *tip, const char *mark, int timeout){
	if(!testname || !buf || !tip || !mark)
		return -1;
	
	const char *label[] = {"msgid", "cmd", "name", "tip", "mark", "timeout", NULL};

	char label_val[6][LABEL_VAL_LEN_MAX];
	snprintf(label_val[0], LABEL_VAL_LEN_MAX-1, "%d", id);
	strncpy(label_val[1], "select", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[2], testname, LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[3], tip, LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[4], mark, LABEL_VAL_LEN_MAX-1);
	snprintf(label_val[5], LABEL_VAL_LEN_MAX-1, "%d", timeout);

	struct xmlNode baseNode[7];
	buildXmlNodeInfoNew(label, label_val, baseNode);

	int len = 1024;
	if(limit<len){
		ERROR("limit(%d)<len(%d)\n", limit, len);
		return -1;
	}
	memset(buf, 0, limit);

	CreateXmlPacket("req", baseNode, ARRAY_SIZE(baseNode), buf);

	return strlen(buf);
}

/*********************************************************************
 * name: fill_packet_operator
 * input:
 * testname: testcase name
 * id: msg id
 * limit: max lenth of packet
 * plugin: plugin
 * datatype: datatype
 * data: data
 * input && output:
 * buf: buffer of packet
 * return: lenth of packet
*********************************************************************/
int fill_packet_operator(char *buf, int limit, const char *testname, int id, const char *plugin, const char *datatype, const char *data){
	if(!testname || !buf || !plugin || !datatype || !data)
		return -1;
	
	const char *label[] = {"msgid", "cmd", "name", "tip", "plugin", "datatype", "data", NULL};

	char label_val[7][LABEL_VAL_LEN_MAX];
	snprintf(label_val[0], LABEL_VAL_LEN_MAX-1, "%d", id);
	strncpy(label_val[1], "operator", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[2], testname, LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[3], "operate start", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[4], plugin, LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[5], datatype, LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[6], data, LABEL_VAL_LEN_MAX-1);

	struct xmlNode baseNode[8];
	buildXmlNodeInfoNew(label, label_val, baseNode);

	int len = 1024;
	if(limit<len){
		ERROR("limit(%d)<len(%d)\n", limit, len);
		return -1;
	}
	memset(buf, 0, limit);

	CreateXmlPacket("req", baseNode, ARRAY_SIZE(baseNode), buf);

	return strlen(buf);
}

/*********************************************************************
 * name: fill_packet_edit
 * input:
 * testname: testcase name
 * id: msg id
 * limit: max lenth of packet
 * tip: tip to show
 * editvalue: editvalue
 * mark: mark
 * timeout: timeout
 * input && output:
 * buf: buffer of packet
 * return: lenth of packet
*********************************************************************/
int fill_packet_edit(char *buf, int limit, const char *testname, int id, const char *tip, const char *editvalue, const char *mark, int timeout){
	if(!testname || !buf || !tip || !editvalue || !mark)
		return -1;
	
	const char *label[] = {"msgid", "cmd", "name", "tip", "editvalue", "mark", "timeout", NULL};
	char label_val[7][LABEL_VAL_LEN_MAX];
	snprintf(label_val[0], LABEL_VAL_LEN_MAX-1, "%d", id);
	strncpy(label_val[1], "edit", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[2], testname, LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[3], tip, LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[4], "\0", LABEL_VAL_LEN_MAX-1);
	strncpy(label_val[5], mark, LABEL_VAL_LEN_MAX-1);
	snprintf(label_val[6], LABEL_VAL_LEN_MAX-1, "%d", timeout);

	struct xmlNode baseNode[8];
	buildXmlNodeInfoNew(label, label_val, baseNode);

	int len = 1024;
	if(limit<len){
		ERROR("limit(%d)<len(%d)\n", limit, len);
		return -1;
	}
	memset(buf, 0, limit);

	CreateXmlPacket("req", baseNode, ARRAY_SIZE(baseNode), buf);

	return strlen(buf);
}

/*********************************************************************
* name: CreateXmlPacket
* function: create xml packet
* input: type - req or request, baseNode - xml node information
* return: xmlBuffer - xml packet
*********************************************************************/
void CreateXmlPacket(char *type, struct xmlNode *baseNode, int len, char *xmlBuffer)
{
	int i = 0;

	strcpy(xmlBuffer, "<");
	strcat(xmlBuffer, type);
	strcat(xmlBuffer, ">");

	for (i = 0; baseNode[i].name[0] != '\0'; i++)
	{
		strcat(xmlBuffer, "<");
		strcat(xmlBuffer, baseNode[i].name);
		strcat(xmlBuffer, ">");

		strcat(xmlBuffer, baseNode[i].content);

		strcat(xmlBuffer, "</");
		strcat(xmlBuffer, baseNode[i].name);
		strcat(xmlBuffer, ">");
	}

	strcat(xmlBuffer, "</");
	strcat(xmlBuffer, type);
	strcat(xmlBuffer, ">");
}

/*********************************************************************
 * * name: GetXmlNode
 * * function: get xml node information from xml packet
 * * input: xmlBuffer - xml packet, type - req or response
 * * return: node - node name, nodeBuffer - node value
 * *********************************************************************/
//<response><msgid>105</msgid><result>ok</result></response>
int GetXmlNode(char *xmlBuffer, char *type, char *node, char *nodeBuffer)
{
	int i = 0;
	char name[20] = {0};
	char *p, *start, *end;

	if (strstr(xmlBuffer, type) == NULL) {
		ERROR("get type err\n");
		return -1;
	}

	strcpy(name, "<");
	strcat(name, node);
	strcat(name, ">");
	start = strstr(xmlBuffer, name);
	if(start == NULL) {
		ERROR("get %s err\n", name);
		return -1;
	}
	start += strlen(name);

	strcpy(name, "</");
	strcat(name, node);
	strcat(name, ">");
	end = strstr(xmlBuffer, name);
	if(start == NULL) {
		ERROR("get %s err\n", name);
		return -1;
	}

	for (p = start; p < end; p++)
		nodeBuffer[i++] = *p;

	nodeBuffer[i] = '\0';

	return 0;
}
