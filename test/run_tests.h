//
//  run_tests.h
//  libms
//
//  Created by Jianguo Wu on 2018/11/28.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef run_tests_h
#define run_tests_h

// TODO:
//  404
//  301, 302
//  HEAD
//  socket closed
//  task

// async tests
typedef void (*on_case_done)(void);


#define TEST_CASE_MAP(XX)                           \
        XX(SERVER_1, server_1)                      \
        XX(SERVER_2, server_2)                      \
        XX(SERVER_3, server_3)                      \
        XX(SERVER_4, server_4)                      \
        XX(SERVER_5, server_5)                      \
        XX(SERVER_6, server_6)                      \


#define GEN_TEST_ENTRY(i, f) void test_##f(on_case_done);
TEST_CASE_MAP(GEN_TEST_ENTRY)
#undef GEN_TEST_ENTRY

void run_async_tests(const char *path);


// sync tests

#define SYNC_TEST_CASE_MAP(XX)              \
      XX(MEM_STORAGE,  mem_storage)         \
      XX(FILE_STORAGE, file_storage)        \
      XX(TASK_1, task_1)                    \
      XX(TASK_2, task_2)                    \

// #define SYNC_TEST_CASE_MAP(XX)     \
//         XX(TASK_1, task_1)         \
//         XX(TASK_2, task_2)         \


#define GEN_SYNC_TEST_ENTRY(i, f) void test_##f(void);
SYNC_TEST_CASE_MAP(GEN_SYNC_TEST_ENTRY)
#undef GEN_SYNC_TEST_ENTRY


void run_sync_tests(const char *path);


#endif /* run_tests_h */

