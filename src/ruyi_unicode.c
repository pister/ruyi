//
//  ruyi_unicode.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/18.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_unicode.h"
#include <stdio.h>
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
