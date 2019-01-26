//
//  fake_reader.h
//  libms
//
//  Created by Jianguo Wu on 2019/1/19.
//  Copyright © 2019 wujianguo. All rights reserved.
//

#ifndef fake_reader_h
#define fake_reader_h

#include "ms.h"
#include "ms_server.h"
#include "fake_file.h"

struct ms_fake_reader {
  struct ms_ireader       reader;
  struct ms_itask         *task;

  int64_t                 filesize;
  struct ms_fake_file     *file;
  
  void (*on_complete)(struct ms_ireader *reader, int code);
};

struct ms_fake_reader *fake_reader_open(int64_t pos, int64_t len, int64_t filesize);

int64_t fake_reader_start(struct ms_fake_reader *reader);

int64_t fake_reader_read(struct ms_fake_reader *reader, int64_t len);

void fake_reader_close(struct ms_fake_reader *reader);

#endif /* fake_reader_h */
