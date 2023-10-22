/**
 * Copyright (C) 2018 Alibaba.inc, All rights reserved.
 *
 * @file:    agcts_cpp_test.cpp
 * @brief:
 * @author:  tanyan.lk@alibaba-inc.com
 * @date:    2019/7/10
 * @version: 1.0
 */

#include "agcts.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
// checkpoint: make sure cpp virtual function supported
// make sure list, vector, map supported
using namespace std;

class _AGCTSBaseClass
{
public:
    virtual int func1();
};

class _AGCTSSubClass : public _AGCTSBaseClass
{
public:
    virtual int func1();
};

int _AGCTSBaseClass::func1()
{
    _AGCTS_LOGD("base class\n");
    return _AGCTS_FAIL;
}

int _AGCTSSubClass::func1()
{
    _AGCTS_LOGD("sub class\n");
    return _AGCTS_SUCCESS;
}

int _agcts_cpp_virtual_func()
{
    // class, virtual function
    _AGCTSBaseClass *base = new _AGCTSSubClass;
    return base->func1();
}

int _agcts_cpp_list()
{
    int ret = _AGCTS_FAIL;
    // list
    list<int *> *test_list = NULL;
    test_list = new list<int *>();
    list<int *>::iterator iter_list;
    int node0 = 0;
    int node1 = 1;

    test_list->push_back(&node0);
    test_list->push_back(&node1);

    _AGCTS_LOGD("list size:%lu\n", test_list->size());

    if (test_list->size() == 2)
        ret = _AGCTS_SUCCESS;

    for (iter_list = test_list->begin(); iter_list != test_list->end(); iter_list++) {
        int* node = *iter_list;
        _AGCTS_LOGD("list node:%d\n", *node);
    }
    test_list->clear();
    delete test_list;

    return ret;
}

int _agcts_cpp_map()
{
    int ret = _AGCTS_FAIL;
    map<string, int> test_map;
    test_map.insert({"false", 0});
    test_map.insert({"true", 1});

    map<string, int>::iterator iter_map = test_map.find("true");
    int value = iter_map->second;
    _AGCTS_LOGD("value of true is:%d\n", value);

    if (value == 1)
        ret = _AGCTS_SUCCESS;

    return ret;
}

int _agcts_cpp_vector()
{
    int ret = _AGCTS_FAIL;
    int i = 0;
    vector<int> test_vector;

    test_vector.push_back(1);
    test_vector.push_back(3);
    test_vector.push_back(2);
    sort(test_vector.begin(), test_vector.end(), greater<int>());

    for (i = 0; i < test_vector.size(); i++) {
        _AGCTS_LOGD("test_vector[%d]:%d\n", i, test_vector[i]);
    }

    if (test_vector[0] == 3)
        ret = _AGCTS_SUCCESS;

    return ret;
}

AGCTS_TEST_CASE agcts_cpp_test_cases[] = {
    {(char *)"c++ virtual function", _agcts_cpp_virtual_func},
    {(char *)"c++ list", _agcts_cpp_list},
    {(char *)"c++ map", _agcts_cpp_map},
    {(char *)"c++ vector", _agcts_cpp_vector},
};

void agcts_cpp_test()
{
    _AGCTS_TEST_BEGIN

    int i;
    for (i = 0; i < sizeof(agcts_cpp_test_cases) / sizeof(AGCTS_TEST_CASE); i++) {
        agcts_run_test(&agcts_cpp_test_cases[i]);
    }

    _AGCTS_TEST_END
}
