//
//  ms.h
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef ms_h
#define ms_h

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#include "mongoose.h"
#pragma clang diagnostic pop

#include "queue.h"

// TODO: memory leak check
#ifndef MS_MALLOC
#define MS_MALLOC malloc
#endif

#ifndef MS_REALLOC
#define MS_REALLOC realloc
#endif

#ifndef MS_FREE
#define MS_FREE free
#endif


#ifndef MS_DBG
#define MS_DBG(format, ...)  printf("[d:%s:%d] " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#ifndef MS_ASSERT
#define MS_ASSERT assert
#endif

#define MS_VERSION_MAJOR 0
#define MS_VERSION_MINOR 1
#define MS_VERSION_PATCH 0
#define MS_VERSION_IS_RELEASE 0
#define MS_VERSION_SUFFIX "dev"

#define MS_VERSION_HEX  ((MS_VERSION_MAJOR << 16) | \
                         (MS_VERSION_MINOR <<  8) | \
                         (MS_VERSION_PATCH))


unsigned int ms_version(void);
const char* ms_version_string(void);


struct ms_server {
  struct mg_mgr mgr;
  struct mg_connection *nc;
  char    port[16];
  char    path[MG_MAX_PATH];
  
  QUEUE   sessions;
  QUEUE   tasks;
};

char *ms_str_of_ev(int ev);

struct ms_server *ms_default_server(void);

void ms_start(const char *http_port, const char *path, void (*callback)(void));

void ms_stop(void);

void ms_asnyc_start(void);


struct ms_url_param {
  char    *url;
  char    *path;
};


int ms_generate_url(struct ms_url_param *input, char *out_url, size_t out_len);



#endif /* ms_h */


