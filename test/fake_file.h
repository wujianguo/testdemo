//
//  fake_file.h
//  libms
//
//  Created by Jianguo Wu on 2018/12/23.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#ifndef fake_file_h
#define fake_file_h

#include "ms.h"

struct ms_fake_file {
  uint64_t filesize;
};

struct ms_fake_file *open_fake_file(uint64_t filesize);

size_t read_fake(struct ms_fake_file *file, char *buf, size_t nbyte, off_t offset);

void validate_buf(struct ms_fake_file *file, char *buf, size_t nbyte, off_t offset);

int close_fake_file(struct ms_fake_file *file);

#endif /* fake_file_h */
