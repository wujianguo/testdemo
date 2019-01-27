//
//  ms_file_storage.h
//  libms
//
//  Created by Jianguo Wu on 2018/12/5.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef ms_file_storage_h
#define ms_file_storage_h

#include "ms_server.h"
#include "ms_memory_pool.h"

struct ms_file_storage {
  struct ms_istorage st;
  int64_t filesize;
  int     fp;
};

struct ms_file_storage *ms_file_storage_open(const char *path);

#endif /* ms_file_storage_h */

