//
//  ms_task.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms_task.h"
//#include "ms_http_pipe.h"
//#include "ms_mem_storage.h"
//#include "ms_file_storage.h"
#include "ms_memory_pool.h"


static void dispatch_reader(struct ms_task *task, struct ms_ireader *reader);
static void dispatch_pipe(struct ms_task *task, struct ms_ipipe *pipe);

static int64_t max_len_from(struct ms_ireader *reader) {
  return 30*1024*1024;
}

static struct ms_task *cast_from(void *data) {
  return (struct ms_task *)data;
}

static struct ms_ipipe *nearest_pipe(struct ms_task *task, int64_t pos, int direction) {
  QUEUE *q;
  struct ms_ipipe *pipe = NULL, *curr_pipe = NULL;
  QUEUE_FOREACH(q, &task->pipes) {
    pipe = QUEUE_DATA(q, struct ms_ipipe, node);
    if (direction * (pipe->get_current_pos(pipe) - pos) >= 0) {
      if (curr_pipe == NULL) {
        curr_pipe = pipe;
      } else {
        if (direction * (pipe->get_current_pos(pipe) - curr_pipe->get_current_pos(curr_pipe)) < 0) {
          curr_pipe = pipe;
        }
      }
    }
  }
  return curr_pipe;
}

static struct ms_ireader *nearest_reader(struct ms_task *task, int64_t pos, int direction) {
  QUEUE *q;
  struct ms_ireader *reader = NULL, *curr_reader = NULL;
  QUEUE_FOREACH(q, &task->readers) {
    reader = QUEUE_DATA(q, struct ms_ireader, node);
    if (direction * (reader->pos + reader->sending - pos) >= 0) {
      if (curr_reader == NULL) {
        curr_reader = reader;
      } else {
        if (direction * (reader->pos + reader->sending - (curr_reader->pos + curr_reader->sending)) < 0) {
          curr_reader = reader;
        }
      }
    }
  }
  return curr_reader;
}


static int64_t get_filesize(struct ms_ipipe *pipe) {
  struct ms_task *task = cast_from(pipe->user_data);
  return task->storage->get_filesize(task->storage);
}

static void on_header(struct ms_ipipe *pipe, struct http_message *hm) {
  struct ms_task *task = cast_from(pipe->user_data);
  struct mg_str *type = mg_get_http_header(hm, "Content-Type");
  MS_ASSERT(type);
  // TODO: if type == nil
  task->content_type = mg_strdup_nul(*type);
}

static void on_filesize(struct ms_ipipe *pipe, int64_t filesize) {
  struct ms_task *task = cast_from(pipe->user_data);
  task->storage->set_filesize(task->storage, filesize);
  QUEUE *q;
  struct ms_ireader *reader = NULL;
  QUEUE_FOREACH(q, &task->readers) {
    reader = QUEUE_DATA(q, struct ms_ireader, node);
    reader->on_filesize(reader, filesize);
  }
}

static void on_content_size(struct ms_ipipe *pipe, int64_t pos, int64_t size) {
  struct ms_task *task = cast_from(pipe->user_data);
  
  if (pipe->get_req_len(pipe) == 0) {
    task->storage->set_filesize(task->storage, pos + size);
  } else {
    task->storage->set_content_size(task->storage, pos, size);
  }
  
  QUEUE *q;
  struct ms_ireader *reader = NULL;
  QUEUE_FOREACH(q, &task->readers) {
    reader = QUEUE_DATA(q, struct ms_ireader, node);
    reader->on_content_size_from(reader, pos, size);
  }
}

static void on_recv(struct ms_ipipe *pipe, const char *buf, int64_t pos, size_t len) {
  struct ms_task *task = cast_from(pipe->user_data);
  // TODO: 淘汰缓存
  size_t write = task->storage->write(task->storage, buf, pos, len);
  MS_ASSERT(write == len);
  //    ms_http_pipe_forward(pipe, write);
  
  QUEUE *q;
  struct ms_ireader *reader = NULL;
  QUEUE_FOREACH(q, &task->readers) {
    reader = QUEUE_DATA(q, struct ms_ireader, node);
    reader->on_recv(reader, pos, len); // TODO: on_recv maybe remove this pipe?
  }
  
  dispatch_pipe(task, pipe);
}

static void on_pipe_complete(struct ms_ipipe *pipe) {
  QUEUE_REMOVE(&pipe->node);
  pipe->close(pipe);
//  ms_http_pipe_close(pipe);
}

static void on_close(struct ms_ipipe *pipe, int code) {
  QUEUE_REMOVE(&pipe->node);
  pipe->close(pipe);
//  ms_http_pipe_close(pipe);
}

static void create_pipe_for(struct ms_task *task, struct ms_ireader *reader, int64_t pos, int64_t len) {
  struct ms_ipipe_callback callback = {
    get_filesize,
    on_header,
    on_filesize,
    on_content_size,
    on_recv,
    on_pipe_complete,
    on_close
  };
  
//  struct ms_ipipe *pipe = (struct ms_ipipe *)ms_http_pipe_create(task->url, pos, len, callback);
  struct ms_ipipe *pipe = task->factory.open_pipe(task->url, pos, len, callback);
//  QUEUE_INIT(&pipe->node);
  QUEUE_INSERT_TAIL(&task->pipes, &pipe->node);
  pipe->user_data = task;
//  pipe->on_content_size = on_content_size;
//  pipe->on_recv = on_recv;
//  pipe->get_filesize = get_filesize;
//  pipe->on_complete = on_pipe_complete;
//  pipe->on_close = on_close;

  pipe->connect(pipe);
//  ms_http_pipe_connect(pipe);
}

static void dispatch_reader(struct ms_task *task, struct ms_ireader *reader) {
  // create pipe if need
  
  // TODO: req_len == 0
  int64_t len = reader->req_len;
  if (reader->req_len > 0) {
    len = reader->req_len + reader->req_pos % MS_PIECE_UNIT_SIZE;
  }
  
  int64_t pos = reader->req_pos - reader->req_pos % MS_PIECE_UNIT_SIZE;
  if (task->storage->get_estimate_size(task->storage) > 0) {
    task->storage->cached_from(task->storage, reader->pos + reader->sending, &pos, &len);
    if (len == 0) {
      return;
    }
  }
  
  struct ms_ipipe *near_pipe = nearest_pipe(task, pos, 1);
  if (near_pipe && near_pipe->get_current_pos(near_pipe) == pos) {
    return;
  }
  
  /*
  if (pos >= reader->pos + max_len_from(reader)) {
    return;
  }

  if (task->storage->get_filesize(task->storage) > 0) {
    if (pos + len >= reader->pos + max_len_from(reader) || len == 0) {
      len = reader->pos + max_len_from(reader) - pos;
      if (pos + len > task->storage->get_filesize(task->storage)) {
        len = 0;
      }
    }
  }
  */
  create_pipe_for(task, reader, pos, len);
}

static void dispatch_pipe(struct ms_task *task, struct ms_ipipe *pipe) {
  // remove pipe if need
  
  QUEUE *q;
  struct ms_ipipe *temp = NULL;
  QUEUE_FOREACH(q, &task->pipes) {
    temp = QUEUE_DATA(q, struct ms_ipipe, node);
    if (temp != pipe && temp->get_current_pos(temp) == pipe->get_current_pos(pipe)) {
      QUEUE_REMOVE(&pipe->node);
      pipe->close(pipe);
//      ms_http_pipe_close(pipe);
      return;
    }
  }

  struct ms_ireader *reader = nearest_reader(task, pipe->get_current_pos(pipe), -1);
  
  if (!reader) {
    QUEUE_REMOVE(&pipe->node);
    pipe->close(pipe);
//    ms_http_pipe_close(pipe);
    return;
  }
  
  if (reader->pos + reader->sending + max_len_from(reader) <= pipe->get_current_pos(pipe)) {
    QUEUE_REMOVE(&pipe->node);
    pipe->close(pipe);
//    ms_http_pipe_close(pipe);
  }

  if (pipe->get_current_pos(pipe) > 0) {
    struct ms_ipipe *near_pipe = nearest_pipe(task, pipe->get_current_pos(pipe) - 1, -1);
    if (near_pipe && reader->pos + reader->sending <= near_pipe->get_current_pos(near_pipe)) {
      QUEUE_REMOVE(&pipe->node);
      pipe->close(pipe);
//      ms_http_pipe_close(pipe);
    }
  }

}

static void add_reader(struct ms_itask *task, struct ms_ireader *reader) {
  struct ms_task *t = (struct ms_task *)task;
  QUEUE_INSERT_TAIL(&t->readers, &reader->node);
  dispatch_reader(t, reader);
}

static size_t task_read(struct ms_itask *task, char *buf, int64_t pos, size_t len) {
  // TODO: should dispatch_pipe or not?
  struct ms_task *t = (struct ms_task *)task;
  return t->storage->read(t->storage, buf, pos, len);
}

static int64_t get_task_filesize(struct ms_itask *task) {
  struct ms_task *t = (struct ms_task *)task;
  return t->storage->get_filesize(t->storage);
}

static int64_t get_task_estimate_size(struct ms_itask *task) {
  struct ms_task *t = (struct ms_task *)task;
  return t->storage->get_estimate_size(t->storage);
}

static struct mg_str get_content_type(struct ms_itask *task) {
  struct ms_task *t = (struct ms_task *)task;
  return t->content_type;
}

static void remove_reader(struct ms_itask *task, struct ms_ireader *reader) {
  MS_DBG("task:%p, reader:%p", task, reader);
  struct ms_task *t = (struct ms_task *)task;
  QUEUE_REMOVE(&reader->node);
  if (QUEUE_EMPTY(&t->readers)) {
    
//    ms_task_close(task, 5);
//    task->close(task, 5);
    t->close_ts = mg_time();
  }
}

static void task_close(struct ms_itask *task) {
  MS_DBG("task:%p", task);
  struct ms_task *t = (struct ms_task *)task;
  MS_ASSERT(QUEUE_EMPTY(&t->readers));
  
  QUEUE *q;
  struct ms_ipipe *pipe;
  while (!QUEUE_EMPTY(&t->pipes)) {
    q = QUEUE_HEAD(&t->pipes);
    pipe = QUEUE_DATA(q, struct ms_ipipe, node);
    QUEUE_REMOVE(&pipe->node);
    pipe->close(pipe);
//    ms_http_pipe_close(pipe);
  }

  t->storage->close(t->storage);
  MS_FREE((void *)t->url.p);
  QUEUE_REMOVE(&t->node);
  MS_FREE(t);
}

struct ms_task *ms_task_open(const struct mg_str url, struct ms_factory factory) {
  struct ms_task *task = MS_MALLOC(sizeof(struct ms_task));
  memset(task, 0, sizeof(struct ms_task));
  MS_DBG("task:%p", task);
  QUEUE_INIT(&task->readers);
  QUEUE_INIT(&task->pipes);
  QUEUE_INIT(&task->node);
  task->url = mg_strdup_nul(url);
  task->factory = factory;
  
  //  task->storage = (struct ms_istorage *)ms_file_storage_open();
  //  task->storage = (struct ms_istorage *)ms_mem_storage_open();
  task->storage = task->factory.open_storage();
  task->task.add_reader = add_reader;
  task->task.read = task_read;
  task->task.content_type = get_content_type;
  task->task.get_filesize = get_task_filesize;
  task->task.get_estimate_size = get_task_estimate_size;
  task->task.remove_reader = remove_reader;
  task->task.close = task_close;
  return task;
}
