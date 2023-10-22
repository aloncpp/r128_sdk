#ifndef __XML_PACKET_H
#define __XML_PACKET_H

struct xmlNode{
    char name[20];
    char content[512];
};

/*head file for xml packet*/
int fill_packet_start(char *buf, int limit, const char *testname, int id);
int fill_packet_end(char *buf, int limit, const char *testname, int id, int result, const char *mark);
int fill_packet_finish(char *buf, int limit, int id, int result, const char *mark);
int fill_packet_tip(char *buf, int limit, const char *testname, int id, const char *tip);
int fill_packet_select(char *buf, int limit, const char *testname, int id, const char *tip, const char *mark, int timeout);
int fill_packet_operator(char *buf, int limit, const char *testname, int id, const char *plugin, const char *datatype, const char *data);
int fill_packet_edit(char *buf, int limit, const char *testname, int id, const char *tip, const char *editvalue, const char *mark, int timeout);

void CreateXmlPacket(char *type, struct xmlNode *baseNode, int len, char* xmlBuffer);
int buildXmlNodeInfo(char *nodeName[], char nodeValue[][512], struct xmlNode *baseNode);
int GetXmlNode(char *xmlBuffer, char *type, char *node, char *nodeBuffer);

#endif
