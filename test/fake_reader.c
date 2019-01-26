//
//  fake_reader.c
//  libms
//
//  Created by Jianguo Wu on 2019/1/19.
//  Copyright © 2019 wujianguo. All rights reserved.
//

#include "fake_reader.h"
#include "ms_task.h"

static int64_t try_read(struct ms_ireader *reader, int64_t max) {
  struct ms_fake_reader *fake_reader = (struct ms_fake_reader *)reader;
  char buf[MG_MAX_HTTP_SEND_MBUF] = {0};
  int64_t all_size = 0;
  while (fake_reader->reader.len > 0) {
    size_t wanted = fake_reader->reader.len;
    if (wanted > MG_MAX_HTTP_SEND_MBUF) {
      wanted = MG_MAX_HTTP_SEND_MBUF;
    }
    
    if (max > 0) {
      if (wanted > max - all_size) {
        wanted = max - all_size;
      }
    }
    
    size_t size = fake_reader->task->read(fake_reader->task, buf, fake_reader->reader.pos, wanted);
    validate_buf(fake_reader->file, buf, size, fake_reader->reader.pos);
    fake_reader->reader.pos += size;
    fake_reader->reader.len -= size;
    all_size += size;
    if (size == 0) {
      break;
    }
  }
  if (fake_reader->reader.len == 0) {
    if (fake_reader->on_complete) {
      fake_reader->on_complete(&fake_reader->reader, 0);
    }
  }
  return all_size;
}

static void on_send(struct ms_ireader *reader, int len) {
  
}

static void on_recv(struct ms_ireader *reader, int64_t pos, size_t len) {
  struct ms_fake_reader *fake_reader = (struct ms_fake_reader *)reader;
  MS_ASSERT(len <= 2*1024*1024);
  char buf[2*1024*1024] = {0};
  size_t size = fake_reader->task->read(fake_reader->task, buf, pos, len);
  MS_ASSERT(size == len);
  validate_buf(fake_reader->file, buf, len, pos);
  
//  try_read(reader, 0);
}

static void on_filesize(struct ms_ireader *reader, int64_t filesize) {
  struct ms_fake_reader *fake_reader = (struct ms_fake_reader *)reader;
  MS_ASSERT(fake_reader->filesize == filesize);
}

static void on_content_size_from(struct ms_ireader *reader, int64_t pos, int64_t size) {
  struct ms_fake_reader *fake_reader = (struct ms_fake_reader *)reader;
  MS_ASSERT(fake_reader->filesize >= pos + size);
}

static void on_error(struct ms_ireader *reader, int code) {
  MS_ASSERT(0);
  struct ms_fake_reader *fake_reader = (struct ms_fake_reader *)reader;
  fake_reader->on_complete(&fake_reader->reader, code);
}


struct ms_fake_reader *fake_reader_open(int64_t pos, int64_t len, int64_t filesize) {
  struct ms_fake_reader *fake_reader = (struct ms_fake_reader *)MS_MALLOC(sizeof(struct ms_fake_reader));
  memset(fake_reader, 0, sizeof(struct ms_fake_reader));
  fake_reader->reader.pos = pos;
  fake_reader->reader.len = len;
  fake_reader->reader.req_pos = pos;
  fake_reader->reader.req_len = len;
  fake_reader->reader.on_send = on_send;
  fake_reader->reader.on_recv = on_recv;
  fake_reader->reader.on_filesize = on_filesize;
  fake_reader->reader.on_content_size_from = on_content_size_from;
  fake_reader->reader.on_error = on_error;
  
  fake_reader->filesize = filesize;
  fake_reader->file = open_fake_file(filesize);
  return fake_reader;
}

int64_t fake_reader_start(struct ms_fake_reader *reader) {
  return try_read(&reader->reader, 0);
}

int64_t fake_reader_read(struct ms_fake_reader *reader, int64_t len) {
  return try_read(&reader->reader, len);
}

void fake_reader_close(struct ms_fake_reader *reader) {
  close_fake_file(reader->file);
  MS_FREE(reader);
}
