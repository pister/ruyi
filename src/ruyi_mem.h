//
//  ruyi_mem.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/15.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_mem_h
#define ruyi_mem_h

#include "ruyi_basics.h"


void* ruyi_mem_alloc(UINT32 size);
void ruyi_mem_free(void * pointer);

#endif /* ruyi_mem_h */
