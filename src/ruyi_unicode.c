//
//  ruyi_unicode.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/18.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_unicode.h"
#include <stdio.h>
#include <string.h>
#include "ruyi_mem.h"


static INT32 ruyi_unicode_decode_single_utf8(const BYTE* src, UINT32 src_pos, UINT32 src_len, WIDE_CHAR *out_utf8_char) {
    WIDE_CHAR b0, b1, b2, b3, b4, b5;
    b0 = src[src_pos];
    // first byte
    // UTF-8:   [0xxx xxxx]
    // 0x80 == 0b1000 0000
    if (b0 < 0x80) {
        // single byte code
        *out_utf8_char = b0;
        return 1;
    }
    // UTF-8: [110y yyyy] [10xx xxxx]
    // 0xC0 = 0b1100 0000
    // 0xE0 = 0b1110 0000
    // 0x000007C0 = 0b0000 0000 0000 0000 0000 0111 1100 0000
    // 0x0000003F = 0b0000 0000 0000 0000 0000 0000 0011 1111
    if ((b0 & 0xE0) == 0xC0) {
        if (src_pos + 2 > src_len) {
            return 0;
        }
        b1 = src[src_pos + 1];
        if ((b1 & 0xC0) != 0x80) {
            return 0;
        }
        *out_utf8_char = ((b0 << 6) & 0x000007C0) | (b1 & 0x0000003F);
        return 2;
    }
    
    // UTF-8: [1110 yyyy] [10xx xxxx] [10xx xxxx]
    // 0xF0 = 0b1111 0000
    // 0xE0 = 0b1110 0000
    // 0x0000F000 = 0b0000 0000 0000 0000 1111 0000 0000 0000
    // 0x00000FC0 = 0b0000 0000 0000 0000 0000 1111 1100 0000
    // 0x0000003F = 0b0000 0000 0000 0000 0000 0000 0011 1111
    if ((b0 & 0xF0) == 0xE0) {
        if (src_pos + 3 > src_len) {
            return 0;
        }
        b1 = src[src_pos + 1];
        if ((b1 & 0xC0) != 0x80) {
            return 0;
        }
        b2 = src[src_pos + 2];
        if ((b2 & 0xC0) != 0x80) {
            return 0;
        }
        *out_utf8_char = ((b0 << 12) & 0x0000F000) | ((b1 << 6) & 0x00000FC0) | (b2 & 0x0000003F);
        return 3;
    }
    
    // UTF-8: [1111 0yyy] [10xx xxxx] [10xx xxxx] [10xx xxxx]
    // 0xF8 = 0b1111 10000
    // 0xF0 = 0b1111 00000
    // 0x001C0000 = 0b0000 0000 0001 1100 0000 0000 0000 0000
    // 0x0003F000 = 0b0000 0000 0000 0011 1111 0000 0000 0000
    // 0x00000FC0 = 0b0000 0000 0000 0000 0000 1111 1100 0000
    // 0x0000003F = 0b0000 0000 0000 0000 0000 0000 0011 1111
    if ((b0 & 0xF8) == 0xF0) {
        if (src_pos + 4 > src_len) {
            return 0;
        }
        b1 = src[src_pos + 1];
        if ((b1 & 0xC0) != 0x80) {
            return 0;
        }
        b2 = src[src_pos + 2];
        if ((b2 & 0xC0) != 0x80) {
            return 0;
        }
        b3 = src[src_pos + 3];
        if ((b3 & 0xC0) != 0x80) {
            return 0;
        }
        *out_utf8_char = ((b0 << 18) & 0x001C0000) | ((b1 << 12) & 0x0003F000) | ((b2 << 6) & 0x00000FC0) | (b3 & 0x0000003F);
        return 4;
    }
    // UTF-8: [1111 10yy] [10xx xxxx] [10xx xxxx] [10xx xxxx] [10xx xxxx]
    // 0xFC = 0b1111 1100
    // 0xF8 = 0b1111 1000
    // 0x03000000 = 0b0000 0011 0000 0000 0000 0000 0000 0000
    // 0x00FC0000 = 0b0000 0000 1111 1100 0000 0000 0000 0000
    // 0x0003F000 = 0b0000 0000 0000 0011 1111 0000 0000 0000
    // 0x00000FC0 = 0b0000 0000 0000 0000 0000 1111 1100 0000
    // 0x0000003F = 0b0000 0000 0000 0000 0000 0000 0011 1111
    if ((b0 & 0xF8) == 0xF0) {
        if (src_pos + 5 > src_len) {
            return 0;
        }
        b1 = src[src_pos + 1];
        if ((b1 & 0xC0) != 0x80) {
            return 0;
        }
        b2 = src[src_pos + 2];
        if ((b2 & 0xC0) != 0x80) {
            return 0;
        }
        b3 = src[src_pos + 3];
        if ((b3 & 0xC0) != 0x80) {
            return 0;
        }
        b4 = src[src_pos + 4];
        if ((b4 & 0xC0) != 0x80) {
            return 0;
        }
        *out_utf8_char = ((b0 << 24) & 0x03000000) | ((b1 << 18) & 0x00FC0000) |
        ((b2 << 12) & 0x0003F000) | ((b3 << 6) & 0x00000FC0) | (b4 & 0x0000003F);
        return 5;
    }
    // UTF-8: [1111 110y] [10xx xxxx] [10xx xxxx] [10xx xxxx] [10xx xxxx] [10xx xxxx]
    // 0xFE = 0b1111 1110
    // 0xFC = 0b1111 1100
    // 0x40000000 = 0b0100 0000 0000 0000 0000 0000 0000 0000
    // 0x3F000000 = 0b0011 1111 0000 0000 0000 0000 0000 0000
    // 0x00FC0000 = 0b0000 0000 1111 1100 0000 0000 0000 0000
    // 0x0003F000 = 0b0000 0000 0000 0011 1111 0000 0000 0000
    // 0x00000FC0 = 0b0000 0000 0000 0000 0000 1111 1100 0000
    // 0x0000003F = 0b0000 0000 0000 0000 0000 0000 0011 1111
    if ((b0 & 0xF8) == 0xF0) {
        if (src_pos + 6 > src_len) {
            return 0;
        }
        b1 = src[src_pos + 1];
        if ((b1 & 0xC0) != 0x80) {
            return 0;
        }
        b2 = src[src_pos + 2];
        if ((b2 & 0xC0) != 0x80) {
            return 0;
        }
        b3 = src[src_pos + 3];
        if ((b3 & 0xC0) != 0x80) {
            return 0;
        }
        b4 = src[src_pos + 4];
        if ((b4 & 0xC0) != 0x80) {
            return 0;
        }
        b5 = src[src_pos + 5];
        if ((b5 & 0xC0) != 0x80) {
            return 0;
        }
        *out_utf8_char = ((b0 << 30) & 0x40000000) | ((b1 << 24) & 0x3F000000) | ((b2 << 18) & 0x00FC0000) |
        ((b3 << 12) & 0x0003F000) | ((b4 << 6) & 0x00000FC0) | (b5 & 0x0000003F);
        return 6;
    }
    return 0;
}

UINT32 ruyi_unicode_decode_utf8(const BYTE* src, UINT32 src_len, UINT32 *src_used_count, WIDE_CHAR *out_utf8_buf, UINT32 buf_length) {
    UINT32 src_pos = 0;
    UINT32 dest_pos = 0;
    UINT32 use_bytes_count = 0;
    while (src_pos < src_len && dest_pos < buf_length) {
        use_bytes_count = ruyi_unicode_decode_single_utf8(src, src_pos, src_len, out_utf8_buf+dest_pos);
        if (use_bytes_count == 0) {
            break;
        }
        src_pos += use_bytes_count;
        dest_pos++;
    }
    if (src_used_count) {
        *src_used_count = src_pos;
    }
    return dest_pos;
}

static UINT32 ruyi_unicode_encode_single_utf8(WIDE_CHAR c, BYTE *out_buf, UINT32 buf_length) {
    // 000000-00007F
    // 0x0000007F == 0b0000 0000 0000 0000 0000 0000 0111 1111
    if (c <= 0x0000007F) {
        if (buf_length < 1) {
            return 0;
        }
        *out_buf = (BYTE)c;
        return 1;
    }
    // UTF-8: [110y yyyy] [10xx xxxx], 000080-0007FF
    // 0x000007FF = 0b0000 0000 0000 0000 0000 0111 1111 1111
    // 0xC0 = 0b1100 0000
    // 0x80 = 0b1000 0000
    // 0x000007C0 = 0b0000 0000 0000 0000 0000 0111 1100 0000
    // 0x0000003F = 0b0000 0000 0000 0000 0000 0000 0011 1111
    if (c <= 0x000007FF) { // 2-byte
        if (buf_length < 2) {
            return 0;
        }
        out_buf[0] = (BYTE) (0xC0 | ((0x000007C0 & c) >> 6));
        out_buf[1] = (BYTE) (0x80 | (0x0000003F & c));
        return 2;
    }
    // UTF-8: [1110 yyyy] [10xx xxxx] [10xx xxxx], 000800-00FFFF
    // 0x0000FFFF = 0b0000 0000 0000 0000 1111 1111 1111 1111
    // 0xE0 = 0b1110 0000
    // 0x80 = 0b1000 0000
    // 0x0000F000 = 0b0000 0000 0000 0000 1111 0000 0000 0000
    // 0x00000FC0 = 0b0000 0000 0000 0000 0000 1111 1100 0000
    // 0x0000003F = 0b0000 0000 0000 0000 0000 0000 0011 1111
    if (c <= 0x0000FFFF) { // 3 bytes
        if (buf_length < 3) {
            return 0;
        }
        out_buf[0] = (BYTE) (0xE0 | ((0x0000F000 & c) >> 12));
        out_buf[1] = (BYTE) (0x80 | ((0x00000FC0 & c) >> 6));
        out_buf[2] = (BYTE) (0x80 | (0x0000003F & c));
        return 3;
    }
    // UTF-8: [1111 0yyy] [10xx xxxx] [10xx xxxx] [10xx xxxx], 010000-10FFFF
    // 0x0010FFFF = 0b0000 0000 0001 0000 1111 1111 1111 1111
    // 0x00010000 = 0b0000 0000 0000 0001 0000 0000 0000 0000
    // 0xF0 = 0b1111 0000
    // 0x80 = 0b1000 0000
    // 0x001C0000 = 0b0000 0000 0001 1100 0000 0000 0000 0000
    // 0x0003F000 = 0b0000 0000 0000 0011 1111 0000 0000 0000
    // 0x00000FC0 = 0b0000 0000 0000 0000 0000 1111 1100 0000
    // 0x0000003F = 0b0000 0000 0000 0000 0000 0000 0011 1111
    if (c <= 0x0010FFFF) { // 4 bytes
        out_buf[0] = (BYTE) (0xF0 | ((0x001C0000 & c) >> 18));
        out_buf[1] = (BYTE) (0x80 | ((0x0003F000 & c) >> 12));
        out_buf[2] = (BYTE) (0x80 | ((0x00000FC0 & c) >> 6));
        out_buf[3] = (BYTE) (0x80 | (0x0000003F & c));
        return 4;
    }
    return 0;
}

UINT32 ruyi_unicode_encode_utf8(const WIDE_CHAR* src_utf8, UINT32 src_len, UINT32 *src_used_count, BYTE *out_buf, UINT32 buf_length) {
    UINT32 src_pos = 0;
    UINT32 out_pos = 0;
    UINT32 bytes_count = 0;
    while (src_pos < src_len && out_pos < buf_length ) {
        bytes_count = ruyi_unicode_encode_single_utf8(src_utf8[src_pos], out_buf + out_pos, buf_length-out_pos);
        if (bytes_count == 0) {
            break;
        }
        src_pos++;
        out_pos += bytes_count;
    }
    if (src_used_count) {
        *src_used_count = src_pos;
    }
    return out_pos;
}

WIDE_CHAR ruyi_unicode_wide_char(const BYTE* str) {
    WIDE_CHAR ret;
    if (0 == ruyi_unicode_decode_utf8(str, 6, NULL, &ret, 1)) {
        return 0;
    }
    return ret;
}

UINT32 ruyi_unicode_bytes(WIDE_CHAR c, BYTE *buf, UINT32 buf_length) {
    return ruyi_unicode_encode_utf8(&c, 1, NULL, buf, buf_length);
}

ruyi_unicode_string * ruyi_unicode_string_init_with_capacity(UINT32 capacity) {
    ruyi_unicode_string * unicode_str = ruyi_mem_alloc(sizeof(ruyi_unicode_string));
    unicode_str->length = 0;
    unicode_str->capacity = capacity;
    unicode_str->data = (WIDE_CHAR*)ruyi_mem_alloc(capacity * sizeof(WIDE_CHAR));
    return unicode_str;
}

static void ruyi_unicode_string_growup(ruyi_unicode_string *unicode_str) {
    assert(unicode_str);
    UINT32 new_capacity = (UINT32)(unicode_str->capacity * 1.5) + 64;
    WIDE_CHAR *new_data = (WIDE_CHAR*)ruyi_mem_alloc(new_capacity * sizeof(WIDE_CHAR));
    memcpy(new_data, unicode_str->data, unicode_str->length  * sizeof(WIDE_CHAR));
    ruyi_mem_free(unicode_str->data);
    unicode_str->data = new_data;
    unicode_str->capacity = new_capacity;
}

void ruyi_unicode_string_append_wide_char(ruyi_unicode_string *unicode_str, WIDE_CHAR c) {
    ruyi_unicode_string_append(unicode_str, &c, 1);
}

void ruyi_unicode_string_append(ruyi_unicode_string *unicode_str, WIDE_CHAR *data, UINT32 len) {
    assert(unicode_str);
    UINT32 remain = unicode_str->capacity - unicode_str->length;
    while (remain < len) {
        ruyi_unicode_string_growup(unicode_str);
        remain = unicode_str->capacity - unicode_str->length;
    }
    memcpy(unicode_str->data + unicode_str->length, data, len * sizeof(WIDE_CHAR));
    unicode_str->length += len;
}

void ruyi_unicode_string_append_utf8(ruyi_unicode_string *unicode_str, const char* src, UINT32 len) {
    assert(unicode_str);
    WIDE_CHAR buf[1024];
    UINT32 read_length = 0;
    UINT32 bytes_length = 0;
    UINT32 transform_bytes_length;
    if (len == 0) {
        len = (UINT32)strlen(src);
    }
    while (bytes_length < len) {
        read_length = ruyi_unicode_decode_utf8((const BYTE*)(src + bytes_length), len - bytes_length, &transform_bytes_length, buf, 1024);
        ruyi_unicode_string_append(unicode_str, buf, read_length);
        bytes_length += transform_bytes_length;
    }
}

ruyi_unicode_string * ruyi_unicode_string_init_from_utf8(const char* src, UINT32 len) {
    ruyi_unicode_string * unicode_str;
    if (len == 0) {
        len = (UINT32)strlen(src);
    }
    unicode_str = ruyi_unicode_string_init_with_capacity(len/2 + 1);
    ruyi_unicode_string_append_utf8(unicode_str, src, len);
    return unicode_str;
}

UINT32 ruyi_unicode_string_length(const ruyi_unicode_string *unicode_str) {
    assert(unicode_str);
    return unicode_str->length;
}

WIDE_CHAR ruyi_unicode_string_at(const ruyi_unicode_string *unicode_str, UINT32 index) {
    return unicode_str->data[index];
}

void ruyi_unicode_string_set(ruyi_unicode_string *unicode_str, UINT32 index, WIDE_CHAR c) {
    unicode_str->data[index] = c;
}

void ruyi_unicode_string_destroy(ruyi_unicode_string* s) {
    if (s == NULL) {
        return;
    }
    if (s->data) {
        ruyi_mem_free(s->data);
    }
    ruyi_mem_free(s);
}


static ruyi_bytes_string* ruyi_bytes_string_init_with_capacity(int capacity) {
    ruyi_bytes_string* str = (ruyi_bytes_string*)ruyi_mem_alloc(sizeof(ruyi_bytes_string));
    str->capacity = capacity;
    str->length = 0;
    str->str = (char*)ruyi_mem_alloc(sizeof(char) * capacity);
    return str;
}

void ruyi_unicode_bytes_string_append(ruyi_bytes_string* s, const char *buf, UINT32 buf_len) {
    UINT32 new_capacity;
    char* new_data;
    UINT32 remain = s->capacity - s->length;
    UINT32 str_len = buf_len + 1;
    while (remain < str_len) {
        // start grow up
        new_capacity = (UINT32)(s->capacity * 1.5) + 64;
        new_data = (char*)ruyi_mem_alloc(sizeof(char) * new_capacity);
        memcpy(new_data, s->str, sizeof(char) * s->length);
        ruyi_mem_free(s->str);
        s->str = new_data;
        s->capacity = new_capacity;
        // end grow up
        remain = s->capacity - s->length;
    }
    memcpy(s->str, buf, sizeof(char) * buf_len);
    s->length += buf_len;
    s->str[s->length] = '\0';
}

ruyi_bytes_string* ruyi_unicode_string_decode_utf8(const ruyi_unicode_string *unicode_str) {
    ruyi_bytes_string* str = ruyi_bytes_string_init_with_capacity(256);
    char buf[1024];
    UINT32 wide_char_transformed = 0;
    UINT32 src_pos = 0;
    UINT32 bytes_read;
    while (src_pos < unicode_str->length) {
        bytes_read = ruyi_unicode_encode_utf8(unicode_str->data + src_pos, unicode_str->length - src_pos, &wide_char_transformed, (BYTE*)buf, 1024);
        if (bytes_read == 0) {
            break;
        }
        ruyi_unicode_bytes_string_append(str, buf, bytes_read);
        src_pos += wide_char_transformed;
    }
    return str;
}

void ruyi_unicode_bytes_string_destroy(ruyi_bytes_string* s) {
    if (s == NULL) {
        return;
    }
    if (s->str) {
        ruyi_mem_free(s->str);
    }
    ruyi_mem_free(s);
}

BOOL ruyi_unicode_string_equals(const ruyi_unicode_string *unicode_str, const ruyi_unicode_string *unicode_str_other) {
    UINT32 pos;
    if (unicode_str == unicode_str_other) {
        return TRUE;
    }
    if (unicode_str == NULL || unicode_str_other == NULL) {
        return FALSE;
    }
    if (unicode_str->length != unicode_str_other->length) {
        return FALSE;
    }
    for (pos = 0; pos < unicode_str->length; pos++) {
        if (unicode_str->data[pos] != unicode_str_other->data[pos]) {
            return FALSE;
        }
    }
    return TRUE;
}
