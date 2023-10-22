/**
 * Copyright (C) 2018 Alibaba.inc, All rights reserved.
 *
 * @file:    agcts_main.cpp
 * @brief:
 * @author:  tanyan.lk@alibaba-inc.com
 * @date:    2019/7/10
 * @version: 1.0
 */

#include "agcts.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void agcts_run_test(AGCTS_TEST_CASE *test_case)
{
    if (test_case != NULL) {
        if (test_case->func() == _AGCTS_SUCCESS)
            _AGCTS_LOGI("AliGenie Compatibility Test: %s supported\n", test_case->name);
        else
            _AGCTS_LOGI("AliGenie Compatibility Test: %s not supported\n", test_case->name);
    } else {
        _AGCTS_LOGI("AliGenie Compatibility Test: no test cases yet!\n");
    }
}

extern void agcts_websoc_cli_test_entry(void);

#define _cplusplus
#ifdef _cplusplus
extern "C"{
#endif

int agcts_main()
{
    //agcts_cpp_test();
    //agcts_posix_test();
    //agcts_audio_test();
    //agcts_librws_test();

    return 0;
}

#ifdef _cplusplus
}
#endif
