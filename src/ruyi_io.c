//
//  ruyi_io.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/19.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_io.h"
#include "ruyi_mem.h"

#include "ruyi_unicode.h"
#include <string.h> // for memcpy

#define UNICODE_FILE_READ_BUF_SIZE 1024
#define UNICODE_FILE_WRITE_BUF_SIZE 1024
#define UNICODE_FILE_BUFF_SIZE UNICODE_FILE_READ_BUF_SIZE+6

ruyi_unicode_file* ruyi_io_unicode_file_open(FILE *fp) {
    assert(fp);
    ruyi_unicode_file* file = (ruyi_unicode_file*)ruyi_mem_alloc(sizeof(ruyi_unicode_file));
    file->fp = fp;
    file->buffer_limit = 0;
    file->buffer_pos = 0;
    file->buffer_size = UNICODE_FILE_BUFF_SIZE;
    file->buffer = ruyi_mem_alloc(UNICODE_FILE_BUFF_SIZE);
    return file;
}

void ruyi_io_unicode_file_close(ruyi_unicode_file *file) {
    if (!file) {
        return;
    }
    if (file->buffer) {
        ruyi_mem_free(file->buffer);
    }
    if (file->fp) {
        fclose(file->fp);
    }
    ruyi_mem_free(file);
}

UINT32 ruyi_io_unicode_file_read_utf8(ruyi_unicode_file* file, UINT32* dist_buf, UINT32 buf_length) {
    assert(file);
    BYTE temp[8];
    UINT32 read_count;
    UINT32 utf8_count;
    UINT32 bytes_decoded;
    UINT32 remain = file->buffer_limit - file->buffer_pos;
    while (remain < 6) {
        // move the remain data to the front of the file->buffer
        if (file->buffer_pos > 0 && remain > 0) {
            memcpy(temp, file->buffer + file->buffer_pos, remain);
            memcpy(file->buffer, temp, remain);
        }
        file->buffer_pos = 0;
        file->buffer_limit = remain;
        
        read_count = (UINT32)fread(file->buffer + remain, 1, UNICODE_FILE_READ_BUF_SIZE, file->fp);
        if (read_count == 0) {
            break;
        }
        file->buffer_limit += read_count;
        remain = file->buffer_limit;
    }
    if (remain == 0) {
        return 0;
    }
    if (file->buffer_pos >= file->buffer_limit) {
        return 0;
    }
    utf8_count = ruyi_unicode_decode_utf8(file->buffer + file->buffer_pos, file->buffer_limit - file->buffer_pos, &bytes_decoded, dist_buf, buf_length);
    if (utf8_count == 0) {
        return 0;
    }
    file->buffer_pos += bytes_decoded;
    return utf8_count;
}


UINT32 ruyi_io_write_utf8(FILE* file, const UINT32* src_buf, UINT32 src_length) {
    BYTE buf[UNICODE_FILE_WRITE_BUF_SIZE];
    UINT32 src_pos = 0;
    UINT32 writed_count;
    UINT32 src_used_count;
    UINT32 bytes_length;
    while (src_pos < src_length) {
        bytes_length = ruyi_unicode_encode_utf8(src_buf + src_pos, src_length - src_pos, &src_used_count, buf, UNICODE_FILE_WRITE_BUF_SIZE);
        if (bytes_length == 0) {
            break;
        }
        src_pos += src_used_count;
        writed_count = 0;
        while (writed_count < bytes_length) {
            writed_count += fwrite(buf + writed_count, 1, bytes_length - writed_count, file);
        }
    }
    return src_pos;
}
