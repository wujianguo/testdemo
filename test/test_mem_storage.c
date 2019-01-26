//
//  test_mem_storage.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/28.
//  Copyright © 2018 wujianguo. All rights reserved.
//


#include "ms.h"
#include "run_tests.h"
#include "ms_mem_storage.h"
#include "fake_file.h"

void test_mem_storage(void) {
  uint64_t tail = 12345;
  uint64_t filesize = MS_BLOCK_UNIT_SIZE * 5 + tail;
  struct ms_fake_file *file = open_fake_file(filesize);
  struct ms_istorage *st = (struct ms_istorage *)ms_mem_storage_open();
  
  int64_t wanted_pos = 0;
  int64_t wanted_len = 0;
  st->cached_from(st, 0, &wanted_pos, &wanted_len);
  MS_ASSERT(wanted_pos == 0);
  MS_ASSERT(wanted_len == -1);
  
  char read_buf[MS_PIECE_UNIT_SIZE*4 + 1] = {0};
  size_t read_size = 0;
  st->set_filesize(st, filesize);
  MS_ASSERT(st->get_filesize(st) == filesize);
  read_size = st->read(st, read_buf, 0, MS_PIECE_UNIT_SIZE);
  MS_ASSERT(read_size == 0);
  
  st->cached_from(st, 0, &wanted_pos, &wanted_len);
  MS_ASSERT(wanted_pos == 0);
  MS_ASSERT(wanted_len == 0);
  
  char buf[MS_PIECE_UNIT_SIZE*2] = {0};
  read_fake(file, buf, MS_PIECE_UNIT_SIZE*2, 0);
  st->write(st, buf, 0, MS_PIECE_UNIT_SIZE);
  read_size = st->read(st, read_buf, 0, MS_PIECE_UNIT_SIZE);
  MS_ASSERT(read_size == MS_PIECE_UNIT_SIZE);
  validate_buf(file, read_buf, MS_PIECE_UNIT_SIZE, 0);
  
  st->cached_from(st, 0, &wanted_pos, &wanted_len);
  MS_ASSERT(wanted_pos == 0);
  MS_ASSERT(wanted_len == MS_PIECE_UNIT_SIZE);
  
  st->cached_from(st, 10, &wanted_pos, &wanted_len);
  MS_ASSERT(wanted_pos == 10);
  MS_ASSERT(wanted_len == MS_PIECE_UNIT_SIZE - 10);

  st->cached_from(st, MS_PIECE_UNIT_SIZE, &wanted_pos, &wanted_len);
  MS_ASSERT(wanted_pos == 0);
  MS_ASSERT(wanted_len == 0);
  
  read_fake(file, buf, MS_PIECE_UNIT_SIZE * 4, MS_BLOCK_UNIT_SIZE * 2 - MS_PIECE_UNIT_SIZE * 4);
  st->write(st, buf, MS_BLOCK_UNIT_SIZE * 2 - MS_PIECE_UNIT_SIZE * 4, MS_PIECE_UNIT_SIZE * 4);

  st->cached_from(st, MS_PIECE_UNIT_SIZE, &wanted_pos, &wanted_len);
  MS_ASSERT(wanted_pos == MS_BLOCK_UNIT_SIZE * 2 - MS_PIECE_UNIT_SIZE * 4);
  MS_ASSERT(wanted_len == MS_PIECE_UNIT_SIZE * 4);

  
  read_fake(file, buf, MS_PIECE_UNIT_SIZE * 4, MS_BLOCK_UNIT_SIZE * 2);
  st->write(st, buf, MS_BLOCK_UNIT_SIZE * 2, MS_PIECE_UNIT_SIZE * 4);
  
  st->cached_from(st, MS_PIECE_UNIT_SIZE, &wanted_pos, &wanted_len);
  MS_ASSERT(wanted_pos == MS_BLOCK_UNIT_SIZE * 2 - MS_PIECE_UNIT_SIZE * 4);
  MS_ASSERT(wanted_len == MS_PIECE_UNIT_SIZE * 4 + MS_PIECE_UNIT_SIZE * 4);

  
  read_size = st->read(st, read_buf, MS_BLOCK_UNIT_SIZE * 2 + MS_PIECE_UNIT_SIZE * 2 + tail, MS_PIECE_UNIT_SIZE * 4);
  MS_ASSERT(read_size == MS_PIECE_UNIT_SIZE * 4 - (MS_PIECE_UNIT_SIZE * 2 + tail));
  validate_buf(file, read_buf, read_size, MS_BLOCK_UNIT_SIZE * 2 + MS_PIECE_UNIT_SIZE * 2 + tail);
  
  read_fake(file, buf, tail + MS_PIECE_UNIT_SIZE, filesize - tail - MS_PIECE_UNIT_SIZE);
  st->write(st, buf, filesize - tail - MS_PIECE_UNIT_SIZE, tail + MS_PIECE_UNIT_SIZE);
  st->read(st, read_buf, filesize - tail - MS_PIECE_UNIT_SIZE, MS_PIECE_UNIT_SIZE);
  validate_buf(file, read_buf, MS_PIECE_UNIT_SIZE, filesize - tail - MS_PIECE_UNIT_SIZE);
  
  read_fake(file, buf, tail, filesize - tail);
  st->write(st, buf, filesize - tail, tail);
  st->read(st, read_buf, filesize - tail, tail);
  validate_buf(file, read_buf, tail, filesize - tail);
  
  st->cached_from(st, filesize - tail - 1, &wanted_pos, &wanted_len);
  MS_ASSERT(wanted_pos == filesize - tail - 1);
  MS_ASSERT(wanted_len == tail + 1);

  st->close(st);
  
  
  st = (struct ms_istorage *)ms_mem_storage_open();
  read_size = st->read(st, read_buf, 0, MS_PIECE_UNIT_SIZE);
  MS_ASSERT(read_size == 0);

  st->set_content_size(st, 1024*1024, MS_BLOCK_UNIT_SIZE * 2 + tail);
  MS_ASSERT(st->get_estimate_size(st) == 1024*1024 + MS_BLOCK_UNIT_SIZE * 2 + tail);
  read_fake(file, buf, MS_PIECE_UNIT_SIZE*2, 0);
  st->write(st, buf, 0, MS_PIECE_UNIT_SIZE);
  read_size = st->read(st, read_buf, 0, MS_PIECE_UNIT_SIZE);
  MS_ASSERT(read_size == MS_PIECE_UNIT_SIZE);
  validate_buf(file, read_buf, MS_PIECE_UNIT_SIZE, 0);
  
  st->set_content_size(st, 1024*1024, MS_BLOCK_UNIT_SIZE * 4+tail);
  st->set_filesize(st, filesize - tail - 1);
  st->set_content_size(st, 0, 1024);
  st->set_filesize(st, filesize);
  
  read_fake(file, buf, MS_PIECE_UNIT_SIZE*4, MS_BLOCK_UNIT_SIZE * 2);
  st->write(st, buf, MS_BLOCK_UNIT_SIZE * 2, MS_PIECE_UNIT_SIZE*4);
  
  
  read_fake(file, buf, tail + MS_PIECE_UNIT_SIZE, filesize - tail - MS_PIECE_UNIT_SIZE);
  st->write(st, buf, filesize - tail - MS_PIECE_UNIT_SIZE, tail + MS_PIECE_UNIT_SIZE);
  st->read(st, read_buf, filesize - tail - MS_PIECE_UNIT_SIZE, MS_PIECE_UNIT_SIZE);
  validate_buf(file, read_buf, MS_PIECE_UNIT_SIZE, filesize - tail - MS_PIECE_UNIT_SIZE);
  
  read_fake(file, buf, tail, filesize - tail);
  st->write(st, buf, filesize - tail, tail);
  st->read(st, read_buf, filesize - tail, tail);
  validate_buf(file, read_buf, tail, filesize - tail);
  
  st->cached_from(st, filesize - tail - 1, &wanted_pos, &wanted_len);
  MS_ASSERT(wanted_pos == filesize - tail - 1);
  MS_ASSERT(wanted_len == tail + 1);


  st->close(st);
  close_fake_file(file);
  
  filesize = MS_BLOCK_UNIT_SIZE * 4 - tail;
  file = open_fake_file(filesize);
  st = (struct ms_istorage *)ms_mem_storage_open();
  st->set_filesize(st, filesize);
  read_fake(file, buf, MS_PIECE_UNIT_SIZE - tail, filesize + tail - MS_PIECE_UNIT_SIZE);
  st->write(st, buf, filesize + tail - MS_PIECE_UNIT_SIZE, MS_PIECE_UNIT_SIZE - tail);
  read_size = st->read(st, read_buf, filesize + tail - MS_PIECE_UNIT_SIZE, MS_PIECE_UNIT_SIZE - tail);
  MS_ASSERT(read_size == MS_PIECE_UNIT_SIZE - tail);
  validate_buf(file, read_buf, MS_PIECE_UNIT_SIZE - tail, filesize + tail - MS_PIECE_UNIT_SIZE);

  st->cached_from(st, filesize - MS_PIECE_UNIT_SIZE - 1, &wanted_pos, &wanted_len);
  MS_ASSERT(wanted_pos == filesize + tail - MS_PIECE_UNIT_SIZE );
  MS_ASSERT(wanted_len == MS_PIECE_UNIT_SIZE - tail);

  st->close(st);
  close_fake_file(file);
}


