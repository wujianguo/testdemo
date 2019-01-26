//
//  run_tests.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/28.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "run_tests.h"
#include "ms.h"
#include "fake_server.h"

//typedef void (*start_case_func)(notify_case_done_func on_case_done);

typedef enum {
#define XX(i, f)    CASE_INDEX_##i,
  TEST_CASE_MAP(XX)
#undef XX
  TEST_CASE_NUM
} TEST_CASE_INDEXS;

static void run_case(int index);

#define GEN_TEST_DONE(i,f) static void on_##f##_done() { run_case(CASE_INDEX_##i + 1);}
TEST_CASE_MAP(GEN_TEST_DONE)
#undef GEN_TEST_DONE


static void timer_handler(struct mg_connection *nc, int ev, void *ev_data) {
  if (ev == MG_EV_TIMER) {
    ms_stop();
  }
}


#define GEN_CASE_INFO_FRAME(i, f) case CASE_INDEX_##i: { test_##f(on_##f##_done); break;}
static void run_case(int index)
{
  MS_DBG("%d", index);
  switch (index) {
      TEST_CASE_MAP(GEN_CASE_INFO_FRAME)
      
    default:
    {
      struct mg_connection *nc = mg_add_sock(&ms_default_server()->mgr, INVALID_SOCKET, timer_handler);
      mg_set_timer(nc, mg_time() + 6);
      break;
    }
  }
}
#undef GEN_CASE_INFO_FRAME


static void on_server_start() {
  start_fake_server();
  run_case(0);
}


void run_async_tests(const char *path) {
  ms_start("8090", path, on_server_start);
}



typedef enum {
#define XX(i, f)    SYNC_CASE_INDEX_##i,
  SYNC_TEST_CASE_MAP(XX)
#undef XX
  SYNC_TEST_CASE_NUM
} SYNC_TEST_CASE_INDEXS;


#define GEN_SYNC_CASE_INFO_FRAME(i, f) if (index == SYNC_CASE_INDEX_##i) { test_##f();}
static void run_sync_case(int index)
{
  MS_DBG("%d", index);
  while (index < SYNC_TEST_CASE_NUM) {
    SYNC_TEST_CASE_MAP(GEN_SYNC_CASE_INFO_FRAME)
    index += 1;
  }
}
#undef GEN_SYNC_CASE_INFO_FRAME


void run_sync_tests(const char *path) {
  run_sync_case(0);
}
