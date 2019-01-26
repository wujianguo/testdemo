//
//  main.c
//  libms
//
//  Created by Jianguo Wu on 2018/11/21.
//  Copyright © 2018 wujianguo. All rights reserved.
//

#include "run_tests.h"

int main(int argc, const char * argv[]) {
  run_sync_tests();
  run_async_tests();
  return 0;
}

