//
//  ms_mem_storage.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms_mem_storage.h"
#include "ms_memory_pool.h"

#define MAX_SIZE_PER_SESSION (20*1024*1024)

static int block_num(uint64_t filesize) {
  return ceil(1.0 * filesize / MS_BLOCK_UNIT_SIZE);
}

static int block_index(uint64_t pos) {
  return (int)(pos / MS_BLOCK_UNIT_SIZE);
}

static int piece_index(uint64_t pos) {
  return (int)(pos % MS_BLOCK_UNIT_SIZE / MS_PIECE_UNIT_SIZE);
}

static struct ms_block *malloc_block() {
  struct ms_block *block = (struct ms_block *)MS_MALLOC(sizeof(struct ms_block));
  memset(block, 0, sizeof(struct ms_block));
  return block;
}

static struct ms_mem_storage *cast_from(struct ms_istorage *st) {
  return (struct ms_mem_storage *)st;
}

static int64_t get_filesize(struct ms_istorage *st) {
  struct ms_mem_storage *mem_st = cast_from(st);
  return mem_st->filesize;
}

static void realloc_blocks(struct ms_mem_storage *mem_st, int64_t new_size) {
  int64_t old_size = mem_st->estimate_size;
  if (mem_st->filesize > 0) {
    old_size = mem_st->filesize;
  }
  mem_st->blocks = MS_REALLOC(mem_st->blocks, block_num(new_size) * sizeof(struct ms_block *));
  memset((char *)mem_st->blocks + block_num(old_size) * sizeof(struct ms_block *), 0, (block_num(new_size) - block_num(old_size)) * sizeof(struct ms_block *));
  
}

static void set_filesize(struct ms_istorage *st, int64_t filesize) {
  struct ms_mem_storage *mem_st = cast_from(st);
  MS_ASSERT(filesize > 0);
  MS_ASSERT(filesize >= mem_st->filesize);
  
  realloc_blocks(mem_st, filesize);
  
  mem_st->filesize = filesize;
  mem_st->estimate_size = filesize;
}

static int64_t get_estimate_size(struct ms_istorage *st) {
  struct ms_mem_storage *mem_st = cast_from(st);
  return mem_st->estimate_size;
}

static void set_content_size(struct ms_istorage *st, int64_t from, int64_t size) {
  struct ms_mem_storage *mem_st = cast_from(st);
  if (mem_st->filesize > 0) {
    return;
  }
  if (from + size > mem_st->estimate_size) {
    realloc_blocks(mem_st, from + size);
    mem_st->estimate_size = from + size;
  }
}


static void wanted_pos_from(struct ms_istorage *st, int64_t from, int64_t *pos, int64_t *len) {
  *pos = 0;
  *len = 0;
  struct ms_mem_storage *mem_st = cast_from(st);
  
  if (mem_st->estimate_size == 0) {
    *pos = from;
    *len = -1;
    return;
  }
  
  int index_for_block = block_index(from);
  int index_for_piece = piece_index(from);

  int num = block_num(mem_st->estimate_size);
  int find_first = 0;
  for (; index_for_block < num; ++index_for_block) {
    struct ms_block *block = mem_st->blocks[index_for_block];
    if (!block) {
      if (!find_first) {
        continue;
      }
      MS_ASSERT(*pos + *len <= mem_st->estimate_size);
      return;
    }
    
    if (index_for_block > block_index(from)) {
      index_for_piece = 0;
    }
    
    for (; index_for_piece < MS_PIECE_NUM_OF_PER_BLOCK; ++index_for_piece) {
      struct ms_piece *piece = &block->pieces[index_for_piece];
      if (!piece->buf) {
        if (!find_first) {
          continue;
        }
        MS_ASSERT(*pos <= mem_st->estimate_size);
        if (*pos + *len > mem_st->estimate_size) {
          *len = mem_st->estimate_size - *pos;
        }
        return;
      }
      if (!find_first) {
        *pos = MS_BLOCK_UNIT_SIZE * index_for_block + MS_PIECE_UNIT_SIZE * index_for_piece;
        find_first = 1;
        *len = MS_PIECE_UNIT_SIZE;
        if (*pos < from) {
          *len = MS_PIECE_UNIT_SIZE - (from - *pos);
          *pos = from;
        }
      } else {
        *len += MS_PIECE_UNIT_SIZE;
      }
    }
  }
  
  MS_ASSERT(*pos <= mem_st->estimate_size);
  if (*pos + *len > mem_st->estimate_size) {
    *len = mem_st->estimate_size - *pos;
  }
  
}

static size_t storage_write(struct ms_istorage *st, const char *buf, int64_t pos, size_t len) {
  struct ms_mem_storage *mem_st = cast_from(st);
  MS_ASSERT(pos % MS_PIECE_UNIT_SIZE == 0);
  MS_ASSERT(pos + len == mem_st->estimate_size || len % MS_PIECE_UNIT_SIZE == 0);
  size_t write = 0;
  int index_for_block = block_index(pos);
  while (write < len) {
    struct ms_block *block = mem_st->blocks[index_for_block];
    if (!block) {
      block = malloc_block();
      mem_st->blocks[index_for_block] = block;
    }
    int index_for_piece = piece_index(pos + write);
    while (write < len) {
      struct ms_piece *piece = &block->pieces[index_for_piece];
      if (!piece->buf) {
        piece->buf = ms_malloc_piece_buf();
      }
      size_t write_len = MS_PIECE_UNIT_SIZE;
      if (write_len > len - write) {
        write_len = len - write;
      }
      memcpy(piece->buf, buf + write, write_len);
      write += write_len;
      
      index_for_piece += 1;
      if (index_for_piece >= MS_PIECE_NUM_OF_PER_BLOCK) {
        break;
      }
    }
    
    index_for_block += 1;
    if (index_for_block >= block_num(mem_st->estimate_size)) {
      break;
    }
  }
  MS_DBG("%lld, %zu, write: %zu", pos, len, write);
  return write;
}

static size_t storage_read(struct ms_istorage *st, char *buf, int64_t pos, size_t len) {
  struct ms_mem_storage *mem_st = cast_from(st);
  MS_ASSERT(pos >= 0);
  MS_ASSERT(mem_st->estimate_size == 0 || pos + len <= mem_st->estimate_size);
  if (mem_st->estimate_size == 0 && mem_st->filesize == 0) {
    return 0;
  }
  size_t read = 0;
  int index_for_block = block_index(pos);
  while (read < len) {
    struct ms_block *block = mem_st->blocks[index_for_block];
    if (!block) {
      break;
    }
    int index_for_piece = piece_index(pos+read);
    while (read < len) {
      struct ms_piece *piece = &block->pieces[index_for_piece];
      if (!piece->buf) {
        //                MS_DBG("%lld, %zu, read: %zu", pos, len, read);
        return read;
      }
      int offset = (pos + read) % MS_PIECE_UNIT_SIZE;
      size_t read_len = MS_PIECE_UNIT_SIZE - offset;
      if (read_len > len - read) {
        read_len = len - read;
      }
      memcpy(buf + read, piece->buf + offset, read_len);
      read += read_len;
      
      index_for_piece += 1;
      if (index_for_piece >= MS_PIECE_NUM_OF_PER_BLOCK) {
        break;
      }
    }
    
    index_for_block += 1;
    if (block_num(mem_st->estimate_size) <= index_for_block) {
      break;
    }
  }
  //    MS_DBG("%lld, %zu, read: %zu", pos, len, read);
  return read;
}

static void storage_close(struct ms_istorage *st) {
  struct ms_mem_storage *mem_st = cast_from(st);
  MS_DBG("mem_st:%p", mem_st);
  
  int index_for_block = 0;
  int num = block_num(mem_st->estimate_size);
  for (; index_for_block < num; ++index_for_block) {
    struct ms_block *block = mem_st->blocks[index_for_block];
    if (!block) {
      continue;
    }
    int index_for_piece = 0;
    for (; index_for_piece < MS_PIECE_NUM_OF_PER_BLOCK; ++index_for_piece) {
      struct ms_piece *piece = &block->pieces[index_for_piece];
      if (piece->buf) {
        ms_free_piece_buf(piece->buf);
        piece->buf = (char *)0;
      }
    }
  }
  MS_FREE(mem_st->blocks);
  MS_FREE(mem_st);
}


struct ms_mem_storage *ms_mem_storage_open() {
  struct ms_mem_storage *mem_st = (struct ms_mem_storage *)MS_MALLOC(sizeof(struct ms_mem_storage));
  memset(mem_st, 0, sizeof(struct ms_mem_storage));
  
  mem_st->st.get_filesize = get_filesize;
  mem_st->st.set_filesize = set_filesize;
  mem_st->st.get_estimate_size = get_estimate_size;
  mem_st->st.set_content_size = set_content_size;
  mem_st->st.cached_from = wanted_pos_from;
  mem_st->st.write = storage_write;
  mem_st->st.read = storage_read;
  mem_st->st.close = storage_close;
  
  MS_DBG("mem_st:%p", mem_st);
  return mem_st;
}

