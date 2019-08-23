//
//  ruyi_error.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/23.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_error_h
#define ruyi_error_h

#include "ruyi_basics.h"

typedef struct {
    UINT32 line;
    UINT32 column;
    UINT32 width;
} ruyi_error;



#endif /* ruyi_error_h */
