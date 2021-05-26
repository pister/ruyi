//
//  ruyi_mem.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/15.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_mem.h"
#include <stdlib.h>
#include <stdio.h>


void* ruyi_mem_alloc(UINT32 size) {
    void * ptr = malloc(size);
    assert(ptr);
    return ptr;
}

void ruyi_mem_free(void * pointer) {
    if (!pointer) {
        return;
    }
    free(pointer);
}
