//
//  fake_pipe.c
//  ms_test
//
//  Created by Jianguo Wu on 2019/1/23.
//  Copyright © 2019 wujianguo. All rights reserved.
//

#include "fake_pipe.h"

static int64_t get_req_len(struct ms_ipipe *pipe) {
  struct ms_fake_pipe *fake_pipe = (struct ms_fake_pipe *)pipe;
  return fake_pipe->req_len;
}

static int64_t get_current_pos(struct ms_ipipe *pipe) {
  struct ms_fake_pipe *fake_pipe = (struct ms_fake_pipe *)pipe;
  return fake_pipe->pos;
}

static void pipe_connect(struct ms_ipipe *pipe) {
  struct ms_fake_pipe *fake_pipe = (struct ms_fake_pipe *)pipe;
  if (fake_pipe->on_connect) {
    fake_pipe->on_connect(pipe);
  }
}

static void pipe_close(struct ms_ipipe *pipe) {
  struct ms_fake_pipe *fake_pipe = (struct ms_fake_pipe *)pipe;
  if (fake_pipe->on_close) {
    fake_pipe->on_close(pipe, 0);
  }
  close_fake_file(fake_pipe->file);
  MS_FREE(fake_pipe);
}

struct ms_fake_pipe *fake_pipe_open(const struct mg_str url,
                                    int64_t pos,
                                    int64_t len,
                                    struct ms_ipipe_callback callback) {
  struct ms_fake_pipe *fake_pipe = (struct ms_fake_pipe *)MS_MALLOC(sizeof(struct ms_fake_pipe));
  memset(fake_pipe, 0, sizeof(struct ms_fake_pipe));
  QUEUE_INIT(&fake_pipe->pipe.node);
  fake_pipe->pos = pos;
  fake_pipe->len = len;
  fake_pipe->req_len = len;
  
  fake_pipe->pipe.get_req_len = get_req_len;
  fake_pipe->pipe.get_current_pos = get_current_pos;
  fake_pipe->pipe.connect = pipe_connect;
  fake_pipe->pipe.close = pipe_close;
  fake_pipe->pipe.callback = callback;
  
  struct mg_str scheme, user_info, host, path, query, fragment;
  unsigned int port;
  mg_parse_uri(url, &scheme, &user_info, &host, &port, &path, &query, &fragment);
  char filesize_str[16] = {0};
  mg_get_http_var(&query, "filesize", filesize_str, MG_MAX_HTTP_REQUEST_SIZE);
  int64_t filesize = 0;
  sscanf(filesize_str, "%" INT64_FMT, &filesize);
  fake_pipe->file = open_fake_file(filesize);
  
  return fake_pipe;
}

void fake_pipe_error(struct ms_fake_pipe *pipe) {
  pipe->pipe.callback.on_close(&pipe->pipe, 4);
}

void fake_pipe_recv(struct ms_fake_pipe *pipe, size_t len) {
  MS_ASSERT(pipe->len >= len);
  MS_ASSERT(len <= 1024*1024);
  char buf[1024*1024] = {0};
  read_fake(pipe->file, buf, len, pipe->pos);
  pipe->pos += len;
  pipe->pipe.callback.on_recv(&pipe->pipe, buf, pipe->pos - len, len);
}

void fake_pipe_header(struct ms_fake_pipe *pipe) {
  pipe->pipe.callback.on_filesize(&pipe->pipe, pipe->file->filesize);
}

