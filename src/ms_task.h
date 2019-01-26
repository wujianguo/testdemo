//
//  ms_task.h
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef ms_task_h
#define ms_task_h

#include "ms_server.h"

struct ms_task {
  struct ms_itask task;
  QUEUE   node;
  struct mg_str   url;
  struct ms_factory factory;
  struct mg_str   content_type;
  struct ms_istorage *storage;
//  struct mg_connection *timer;
  
  QUEUE   readers;
  QUEUE   pipes;
  
  void    *user_data;
  double  close_ts;
};

struct ms_task *ms_task_open(const struct mg_str url, struct ms_factory factory);

//void ms_task_add_reader(struct ms_task *task, struct ms_ireader *reader);
//
//size_t ms_task_read(struct ms_task *task, char *buf, int64_t pos, size_t len);
//
//int64_t ms_task_filesize(struct ms_task *task);
//
//int64_t ms_task_estimate_size(struct ms_task *task);
//
//void ms_task_remove_reader(struct ms_task *task, struct ms_ireader *reader);
//
//void ms_task_close(struct ms_task *task, double after);

#endif /* ms_task_h */

