//
//  main.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/15.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include <stdio.h>

// test cases
#include "../tests/test_cases.h"

int main(int argc, const char * argv[]) {
    run_test_cases();
    //printf("Hello, World, %lu, %lu, %lu, %lu, %lu\n", sizeof(INT64),sizeof(UINT64), sizeof(INT32), sizeof(UINT32), sizeof(BYTE));
    return 0;
}
