//
//  fake_server.h
//  libms
//
//  Created by Jianguo Wu on 2019/1/4.
//  Copyright © 2019 wujianguo. All rights reserved.
//

#ifndef fake_server_h
#define fake_server_h

#include "ms.h"

enum ms_fake_type {
  ms_fake_type_normal,
  ms_fake_type_close,
  ms_fake_type_error,
  ms_fake_type_non_range,
  ms_fake_type_redirect_1,
  ms_fake_type_redirect_2,
  ms_fake_type_redirect_3,
  ms_fake_type_redirect_4
};

struct ms_fake_nc {
  enum ms_fake_type type;
  char              path[MG_MAX_PATH];
  char              uri[PATH_MAX];
  int64_t           filesize;
  QUEUE             node;
};

struct ms_fake_server {
  QUEUE nc;
};

void start_fake_server(void);

void fake_url(struct ms_fake_nc *nc, char *url, int url_len);

struct ms_fake_nc *nc_of(enum ms_fake_type type);

#endif /* fake_server_h */
