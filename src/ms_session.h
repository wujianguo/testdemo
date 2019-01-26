//
//  ms_session.h
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef ms_session_h
#define ms_session_h


#include "ms_server.h"
//#include "ms_task.h"

struct ms_session {
  struct ms_ireader       reader;
  struct mg_connection    *connection;
  struct ms_itask         *task;
  struct mbuf             buf;
  
  QUEUE                   node;
  
  int                     fp;
};

struct ms_session *ms_session_open(struct mg_connection *nc, struct http_message *hm, struct ms_itask *task);

size_t ms_session_try_transfer_data(struct ms_session *session);

void ms_session_close(struct ms_session *session);


#endif /* ms_session_h */

