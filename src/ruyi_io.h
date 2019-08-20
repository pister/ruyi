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

/**
 * Open an unicode file
 * params:
 * fp - a file pointer, it will be closed when call ruyi_io_unicode_file_close
 */
ruyi_unicode_file* ruyi_io_unicode_file_open(FILE *fp);

/**
 * Close an unicode file
 * params:
 * file - target object
 */
void ruyi_io_unicode_file_close(ruyi_unicode_file *file);

/**
 * Read some unicode data from the file
 * params:
 * file - target object
 * dist_buf - the receive data buffer
 * buf_length - the buffer max length
 * return:
 * the count has been read, 0 indicates end of file or some error ocurrs
 */
UINT32 ruyi_io_unicode_file_read_utf8(ruyi_unicode_file* file, UINT32* dist_buf, UINT32 buf_length);

/**
 * Write some unicode data to the file
 * params:
 * file - target object
 * src_buf - the data buffer need to write
 * buf_length - the length of data need to write
 * return:
 * the count has written, 0 indicates some error ocurrs
 */
UINT32 ruyi_io_write_utf8(FILE* file, const UINT32* src_buf, UINT32 buf_length);

#endif /* ruyi_io_h */
