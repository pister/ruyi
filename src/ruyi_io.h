//
//  ruyi_io.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/19.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_io_h
#define ruyi_io_h

#include <stdio.h>
#include "ruyi_basics.h"

typedef struct {
    FILE* fp;
    BYTE* buffer;
    UINT32 buffer_pos;
    UINT32 buffer_limit;
    UINT32 buffer_size;
} ruyi_unicode_file;

ruyi_unicode_file* ruyi_io_unicode_file_open(FILE *fp);
void ruyi_io_unicode_file_close(ruyi_unicode_file *file);

UINT32 ruyi_io_unicode_file_read_utf8(ruyi_unicode_file* file, UINT32* dist_buf, UINT32 buf_length);

UINT32 ruyi_io_write_utf8(FILE* file, const UINT32* src_buf, UINT32 buf_length);


#endif /* ruyi_io_h */
