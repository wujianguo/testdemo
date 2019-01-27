//
//  ms_file_storage.c
//  libms
//
//  Created by Jianguo Wu on 2018/12/5.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms_file_storage.h"

static struct ms_file_storage *cast_from(struct ms_istorage *st) {
  return (struct ms_file_storage *)st;
}

static int64_t get_filesize(struct ms_istorage *st) {
  struct ms_file_storage *file_st = cast_from(st);
  return file_st->filesize;
}

static void set_filesize(struct ms_istorage *st, int64_t filesize) {
  struct ms_file_storage *file_st = cast_from(st);
  file_st->filesize = filesize;  
}

static int64_t get_estimate_size(struct ms_istorage *st) {
  struct ms_file_storage *file_st = cast_from(st);
  return file_st->filesize;
}

static void set_content_size(struct ms_istorage *st, int64_t from, int64_t size) {
  
}

static void wanted_pos_from(struct ms_istorage *st, int64_t from, int64_t *pos, int64_t *len) {
  *len = 0;
}

static size_t storage_write(struct ms_istorage *st, const char *buf, int64_t pos, size_t len) {
  return 0;
}

static size_t storage_read(struct ms_istorage *st, char *buf, int64_t pos, size_t len) {
  struct ms_file_storage *file_st = cast_from(st);
  return pread(file_st->fp, buf, len, pos);
}

static void storage_close(struct ms_istorage *st) {
  struct ms_file_storage *file_st = cast_from(st);
  MS_DBG("file_st:%p", file_st);
  close(file_st->fp);
  MS_FREE(file_st);
}

struct ms_file_storage *ms_file_storage_open(const char *path) {
  struct ms_file_storage *file_st = (struct ms_file_storage *)MS_MALLOC(sizeof(struct ms_file_storage));
  memset(file_st, 0, sizeof(struct ms_file_storage));
  
  file_st->fp = open(path, O_RDONLY);
  
  file_st->st.get_filesize = get_filesize;
  file_st->st.set_filesize = set_filesize;
  file_st->st.get_estimate_size = get_estimate_size;
  file_st->st.set_content_size = set_content_size;
  file_st->st.cached_from = wanted_pos_from;
  file_st->st.write = storage_write;
  file_st->st.read = storage_read;
  file_st->st.close = storage_close;
  
  MS_DBG("file_st:%p", file_st);
  return file_st;
  
}

