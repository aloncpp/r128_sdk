#ifndef __ASR_RESULT_PARSER_H__
#define __ASR_RESULT_PARSER_H__

#include "cjson/cJSON.h"

#define ASR_RESULT_MUSIC        (1)
#define ASR_RESULT_WEATHER      (2)
#define ASR_RESULT_EAPPLIANCES  (3)
#define ASR_RESULT_LOACL        (4)
#define ASR_RESULT_UNKNOWN      (5)

#define STRING_BUFFER_SIZE  (256)
#define RES_LIST_SIZE       (12)

struct asr_result_t {
    int id;
    cJSON *cjson;
    int status;
    int type;
    char *tts_text;
    char *tts_link;
    char *res_list[RES_LIST_SIZE];
    unsigned int res_num;
};

void asr_result_destroy(struct asr_result_t *hdl);
struct asr_result_t *asr_result_create(const char *value, int id);

#endif /* __ASR_RESULT_PARSER_H__ */