//
//  fake_file.c
//  libms
//
//  Created by Jianguo Wu on 2018/12/23.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "fake_file.h"

static void ms_cs_md5(char buf[33], ...) {
  unsigned char hash[16];
  const uint8_t *msgs[20], *p;
  size_t msg_lens[20];
  size_t num_msgs = 0;
  va_list ap;
  
  va_start(ap, buf);
  while ((p = va_arg(ap, const unsigned char *) ) != NULL) {
    msgs[num_msgs] = p;
    msg_lens[num_msgs] = va_arg(ap, size_t);
    num_msgs++;
  }
  va_end(ap);
  
  mg_hash_md5_v(num_msgs, msgs, msg_lens, hash);
  cs_to_hex(buf, hash, sizeof(hash));
}

struct ms_fake_file *open_fake_file(uint64_t filesize) {
  MS_ASSERT(filesize > 0);
  struct ms_fake_file *file = MS_MALLOC(sizeof(struct ms_fake_file));
  memset(file, 0, sizeof(struct ms_fake_file));
  file->filesize = filesize;
  return file;
}

size_t read_fake(struct ms_fake_file *file, char *buf, size_t nbyte, off_t offset) {
  MS_ASSERT(nbyte + offset <= file->filesize);
  size_t read = 0;
  while (read < nbyte) {
    off_t index = (offset + read) / 32;
    off_t off = (offset + read) % 32;
    
    char index_str[128] = {0};
    snprintf(index_str, 128, "%" INT64_FMT "%" INT64_FMT, index, file->filesize);
    char md5[33];
    ms_cs_md5(md5, index_str, strlen(index_str), NULL);

    if (nbyte - read > 32 - off) {
      memcpy(buf + read, md5 + off, 32 - off);
      read += 32 - off;
    } else {
      memcpy(buf + read, md5 + off, nbyte - read);
      read += nbyte - read;
    }
  }
  
  return read;
}

void validate_buf(struct ms_fake_file *file, char *buf, size_t nbyte, off_t offset) {
  if (nbyte == 0) {
    return;
  }
  char *temp = MS_MALLOC(nbyte);
  size_t read = read_fake(file, temp, nbyte, offset);
  MS_ASSERT(read == nbyte);
  int i = 0;
  for (; i < read; ++i) {
    MS_ASSERT(temp[i] == buf[i]);
  }

  MS_FREE(temp);
}


int close_fake_file(struct ms_fake_file *file) {
  MS_FREE(file);
  return 0;
}
