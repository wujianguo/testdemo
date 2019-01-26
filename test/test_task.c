//
//  test_task.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/28.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "run_tests.h"
#include "ms.h"
#include "ms_task.h"
#include "ms_mem_storage.h"
#include "fake_file.h"
#include "fake_reader.h"
#include "fake_pipe.h"


static struct ms_istorage *open_storage(void) {
  return (struct ms_istorage *)ms_mem_storage_open();
}

struct ms_test_task_case_1 {
  struct ms_task  *task;
  QUEUE           readers;
};

static struct ms_ipipe *open_pipe_case_1(const struct mg_str url,
                                   int64_t pos,
                                   int64_t len,
                                   struct ms_ipipe_callback callback) {
  return (struct ms_ipipe *)fake_pipe_open(url, pos, len, callback);
}


static struct ms_fake_reader *add_reader(struct ms_task *task, int64_t pos, int64_t len, int64_t filesize) {
  struct ms_fake_reader *reader = fake_reader_open(pos, len, filesize);
  reader->task = &task->task;
  task->task.add_reader(&task->task, &reader->reader);
  return reader;
}

void test_task_1() {
//  struct ms_test_task_case_1 *test_case = MS_MALLOC(sizeof(struct ms_test_task_case_1));
//  memset(test_case, 0, sizeof(struct ms_test_task_case_1));
  struct ms_factory factory = {
    open_storage,
    open_pipe_case_1
  };

  int64_t filesize = 1024*1024*300;
  char url[MG_MAX_PATH] = {0};
  snprintf(url, MG_MAX_PATH, "http://127.0.0.1/test.mp4?filesize=%lld", filesize);
  struct ms_task *task = ms_task_open(mg_mk_str(url), factory);
  struct ms_fake_reader *reader1 = add_reader(task, 0, filesize, filesize);
  
  QUEUE *q;
  struct ms_ipipe *pipe = NULL;
  QUEUE_FOREACH(q, &task->pipes) {
    pipe = QUEUE_DATA(q, struct ms_ipipe, node);
    break;
  }
  
  fake_pipe_header((struct ms_fake_pipe *)pipe);
  fake_pipe_recv((struct ms_fake_pipe *)pipe, MS_PIECE_UNIT_SIZE*2);
  fake_reader_start(reader1);
  MS_ASSERT(reader1->reader.pos == MS_PIECE_UNIT_SIZE*2);
  
  struct ms_fake_reader *reader2 = add_reader(task, 0, filesize, filesize);

  task->task.remove_reader(&task->task, &reader1->reader);
  
  
  task->task.remove_reader(&task->task, &reader2->reader);
  task->task.close(&task->task);
  
//  MS_FREE(test_case);
}

void test_task_2() {
  
}
