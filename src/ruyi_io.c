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

UINT32 ruyi_io_unicode_file_read_utf8(ruyi_unicode_file* file, WIDE_CHAR* dist_buf, UINT32 buf_length) {
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


UINT32 ruyi_io_write_utf8(FILE* file, const WIDE_CHAR* src_buf, UINT32 src_length) {
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

static void ruyi_file_growup(ruyi_file* file) {
    BYTE *old_buffer, *new_buffer;
    UINT32 new_capacity;
    if (Ruyi_tf_DATA != file->type) {
        return;
    }
    new_capacity = (UINT32)(file->capacity * 1.5 + 64);
    old_buffer = file->dist.buffer;
    new_buffer = ruyi_mem_alloc(new_capacity);
    memcpy(new_buffer, old_buffer, file->write_pos);
    file->dist.buffer = new_buffer;
    file->capacity = new_capacity;
    ruyi_mem_free(old_buffer);
}

UINT32 ruyi_file_write(ruyi_file* file, const void* buf, UINT32 buf_length) {
    assert(file);
    if (Ruyi_tf_FILE == file->type) {
        return (UINT32)fwrite(buf, 1, buf_length, file->dist.file);
    } else if (Ruyi_tf_DATA == file->type) {
        while (file->write_pos + buf_length > file->capacity) {
            ruyi_file_growup(file);
        }
        memcpy(file->dist.buffer + file->write_pos, buf, buf_length);
        file->write_pos += buf_length;
    }
    return buf_length;
}

UINT32 ruyi_file_read(ruyi_file* file, void* buf, UINT32 buf_length) {
    assert(file);
    UINT32 read_length = 0;
    if (Ruyi_tf_FILE == file->type) {
        return (UINT32)fread(buf, 1, buf_length, file->dist.file);
    } else if (Ruyi_tf_DATA == file->type) {
        read_length = file->write_pos - file->read_pos;
        if (read_length == 0) {
            return 0;
        }
        if (read_length > buf_length) {
            read_length = buf_length;
        }
        memcpy(buf, file->dist.buffer + file->read_pos, read_length);
        file->read_pos += read_length;
    }
    return read_length;
}

ruyi_file* ruyi_file_open_by_file(FILE* file) {
    assert(file);
    ruyi_file* f = (ruyi_file*)ruyi_mem_alloc(sizeof(ruyi_file));
    f->capacity = 0;
    f->read_pos = 0;
    f->write_pos = 0;
    f->type = Ruyi_tf_FILE;
    f->dist.file = file;
    return f;
}

ruyi_file* ruyi_file_init_by_data(const void *data, UINT32 data_length) {
    BYTE *buffer = ruyi_mem_alloc(data_length);
    ruyi_file* f = (ruyi_file*)ruyi_mem_alloc(sizeof(ruyi_file));
    f->capacity = data_length;
    f->read_pos = 0;
    f->write_pos = data_length;
    f->type = Ruyi_tf_DATA;
    memcpy(buffer, data, data_length);
    f->dist.buffer = buffer;
    return f;
}

ruyi_file* ruyi_file_init_by_capacity(UINT32 init_size) {
    BYTE *buffer = ruyi_mem_alloc(init_size);
    ruyi_file* f = (ruyi_file*)ruyi_mem_alloc(sizeof(ruyi_file));
    f->capacity = init_size;
    f->read_pos = 0;
    f->write_pos = 0;
    f->type = Ruyi_tf_DATA;
    f->dist.buffer = buffer;
    return f;
}

void ruyi_file_close(ruyi_file* file) {
    assert(file);
    switch (file->type) {
        case Ruyi_tf_FILE:
            fclose(file->dist.file);
            file->dist.file = NULL;
            break;
        case Ruyi_tf_DATA:
            ruyi_mem_free(file->dist.buffer);
            file->dist.buffer = NULL;
            break;
        default:
            break;
    }
    ruyi_mem_free(file);
}

