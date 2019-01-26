//
//  ms_http_pipe.h
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef ms_http_pipe_h
#define ms_http_pipe_h

#include "ms_server.h"

struct ms_http_pipe {
  struct ms_ipipe pipe;
  
  QUEUE           node;
  
  int             closing;
  //    int             closed;
  
  struct mbuf     buf;
  int64_t         req_pos;
  int64_t         req_len;
  int64_t         pos;
  int64_t         len;
  struct mg_str   url;
  struct mg_connection    *nc;
  
  
  //    int             fp;
};

struct ms_http_pipe *ms_http_pipe_create(const struct mg_str url,
                                         int64_t pos,
                                         int64_t len,
                                         struct ms_ipipe_callback callback);

//void ms_http_pipe_connect(struct ms_http_pipe *pipe);

//void ms_http_pipe_forward(struct ms_http_pipe *pipe, size_t offset);

//void ms_http_pipe_close(struct ms_http_pipe *pipe);


#endif /* ms_http_pipe_h */

