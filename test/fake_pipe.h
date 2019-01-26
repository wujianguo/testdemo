//
//  fake_pipe.h
//  ms_test
//
//  Created by Jianguo Wu on 2019/1/23.
//  Copyright © 2019 wujianguo. All rights reserved.
//

#ifndef fake_pipe_h
#define fake_pipe_h

#include "ms.h"
#include "ms_server.h"
#include "fake_file.h"

struct ms_fake_pipe {
  struct ms_ipipe       pipe;
  struct ms_fake_file   *file;
  
  int64_t pos;
  int64_t len;
  int64_t req_len;
  
  void (*on_connect)(struct ms_ipipe *pipe);
  void (*on_close)(struct ms_ipipe *pipe, int code);
};

struct ms_fake_pipe *fake_pipe_open(const struct mg_str url,
                                    int64_t pos,
                                    int64_t len,
                                    struct ms_ipipe_callback callback);

void fake_pipe_error(struct ms_fake_pipe *pipe);

void fake_pipe_recv(struct ms_fake_pipe *pipe, size_t len);

void fake_pipe_header(struct ms_fake_pipe *pipe);

#endif /* fake_pipe_h */
