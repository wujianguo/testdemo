//
//  main.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "run_tests.h"
#include <assert.h>
#include <unistd.h>

int main(int argc, const char * argv[]) {
  assert(argc == 2);
  chdir(argv[1]);  
  run_sync_tests(argv[1]);
  run_async_tests(argv[1]);
  return 0;
}

