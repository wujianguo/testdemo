//
//  ms_mem_storage.h
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef ms_mem_storage_h
#define ms_mem_storage_h

#include "ms_server.h"
#include "ms_memory_pool.h"

#define MS_BLOCK_UNIT_SIZE (2*1024*1024)
#define MS_PIECE_NUM_OF_PER_BLOCK (MS_BLOCK_UNIT_SIZE/MS_PIECE_UNIT_SIZE)


// 16k
struct ms_piece {
  char    *buf;
};

// 2M
struct ms_block {
  struct ms_piece pieces[MS_PIECE_NUM_OF_PER_BLOCK];
};

struct ms_mem_storage {
  struct ms_istorage st;
  int64_t filesize;
  int64_t estimate_size;
  
  struct ms_block  **blocks;
};

struct ms_mem_storage *ms_mem_storage_open(void);

#endif /* ms_mem_storage_h */

