//
//  test_file_storage.c
//  libms
//
//  Created by Jianguo Wu on 2018/12/23.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "ms.h"
#include "run_tests.h"
#include "ms_file_storage.h"

void test_file_storage(void) {
  char path[MG_MAX_PATH] = {0};
  strcpy(path, ms_default_server()->path);
  strcat(path, "/build/wildo.mp4");

  struct ms_istorage *st = (struct ms_istorage *)ms_file_storage_open(path);

  cs_stat_t cst;
  if (mg_stat(path, &cst) == 0) {
    st->set_filesize(st, cst.st_size);
  } else {
    MS_DBG("%s", path);
  }
  st->set_content_size(st, 0, 1);
  st->get_estimate_size(st);
  st->get_filesize(st);
  int64_t pos = 0, len = 0;
  st->cached_from(st, 0, &pos, &len);
  char buf[MS_PIECE_UNIT_SIZE] = {0};
  st->write(st, buf, 0, 0);
  st->read(st, buf, 0, MS_PIECE_UNIT_SIZE);

  st->close(st);
}

