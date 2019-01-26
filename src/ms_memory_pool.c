//
//  ms_memory_pool.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms_memory_pool.h"

char *ms_malloc_piece_buf() {
  return MS_MALLOC(MS_PIECE_UNIT_SIZE);
}

void ms_free_piece_buf(char *buf) {
  MS_FREE(buf);
}

