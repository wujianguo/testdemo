//
//  ms_memory_pool.h
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef ms_memory_pool_h
#define ms_memory_pool_h

#include "ms_server.h"

#define MS_PIECE_UNIT_SIZE (16*1024)

char *ms_malloc_piece_buf(void);

void ms_free_piece_buf(char *buf);

#endif /* ms_memory_pool_h */

