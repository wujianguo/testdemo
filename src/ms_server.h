//
//  ms_server.h
//  libms
//
//  Created by Jianguo Wu on 2018/11/29.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef ms_server_h
#define ms_server_h

#include "ms.h"

struct ms_istorage {
  int64_t (*get_filesize)(struct ms_istorage *st);
  void    (*set_filesize)(struct ms_istorage *st, int64_t filesize);
  int64_t (*get_estimate_size)(struct ms_istorage *st);
  void    (*set_content_size)(struct ms_istorage *st, int64_t from, int64_t size);
  void    (*cached_from)(struct ms_istorage *st, int64_t from, int64_t *pos, int64_t *len);
  size_t  (*write)(struct ms_istorage *st, const char *buf, int64_t pos, size_t len);
  size_t  (*read)(struct ms_istorage *st, char *buf, int64_t pos, size_t len);
  void    (*close)(struct ms_istorage *st);
};

struct ms_ireader {
  int64_t pos;
  int64_t len;
  size_t  sending;
  size_t  header_sending;
  
  int64_t         req_pos;
  int64_t         req_len;
  
  
  void (*on_send)(struct ms_ireader *reader, int len);
  void (*on_recv)(struct ms_ireader *reader, int64_t pos, size_t len);
  void (*on_filesize)(struct ms_ireader *reader, int64_t filesize);
  void (*on_content_size_from)(struct ms_ireader *reader, int64_t pos, int64_t size);
  //    void (*on_close)(struct ms_reader *reader);
  void (*on_error)(struct ms_ireader *reader, int code);
  
  QUEUE   node;
};

struct ms_itask {
  void (*add_reader)(struct ms_itask *task, struct ms_ireader *reader);
  struct mg_str (*content_type)(struct ms_itask *task);
  size_t (*read)(struct ms_itask *task, char *buf, int64_t pos, size_t len);
  int64_t (*get_filesize)(struct ms_itask *task);
  int64_t (*get_estimate_size)(struct ms_itask *task);
  void (*remove_reader)(struct ms_itask *task, struct ms_ireader *reader);
  void (*close)(struct ms_itask *task);
};

struct ms_ipipe;
struct ms_ipipe_callback {
  int64_t (*get_filesize)(struct ms_ipipe *pipe);
  void    (*on_header)(struct ms_ipipe *pipe, struct http_message *hm);
  void    (*on_filesize)(struct ms_ipipe *pipe, int64_t filesize);
  void    (*on_content_size)(struct ms_ipipe *pipe, int64_t pos, int64_t size);
  void    (*on_recv)(struct ms_ipipe *pipe, const char *buf, int64_t pos, size_t len);
  void    (*on_complete)(struct ms_ipipe *pipe);
  void    (*on_close)(struct ms_ipipe *pipe, int code);
};

struct ms_ipipe {
  QUEUE   node;

  int64_t (*get_req_len)(struct ms_ipipe *pipe);
  int64_t (*get_current_pos)(struct ms_ipipe *pipe);
  void    (*connect)(struct ms_ipipe *pipe);
  void    (*close)(struct ms_ipipe *pipe);
  
  void    *user_data;
  struct ms_ipipe_callback callback;
};

struct ms_idispatch {
  
};

struct ms_factory {
  struct ms_istorage*  (*open_storage)(void);
  struct ms_ipipe*     (*open_pipe)(const struct mg_str url,
                                   int64_t pos,
                                   int64_t len,
                                   struct ms_ipipe_callback callback);
  
};

#endif /* ms_server_h */

