//
//  unicode.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/18.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_unicode_h
#define ruyi_unicode_h

#include "ruyi_basics.h"

/**
 * The utf-8 format
 * see: https://tools.ietf.org/rfc/rfc3629.txt
 */


/**
 * Decode bytes to Unicode by utf-8
 * params:
 * src - the input src
 * src_len - input src lenght
 * src_used_count - to recieve how how many bytes has been decoded, can be NULL
 * out_utf8_buf - the output ucs data
 * buf_length - max output length
 * return:
 * the char count of out_utf8_buf recieved, if occur error, 0 will be return.
 */
UINT32 ruyi_unicode_decode_utf8(const BYTE* src, UINT32 src_len, UINT32 *src_used_count, WIDE_CHAR *out_utf8_buf, UINT32 buf_length);

/**
 * Encode Unicode by utf-8
 * params:
 * src_utf8 - the input utf-8 src ucs
 * src_len - input src lenght
 * src_used_count - to recieve how how many ucs has been encoded, can be NULL
 * out_buf - the output bytes buffer
 * buf_length - max output length
 * return:
 * the char count of out_utf8_buf recieved, if occur error, 0 will be return.
 */
UINT32 ruyi_unicode_encode_utf8(const WIDE_CHAR *src_utf8, UINT32 src_len, UINT32 *src_used_count, BYTE *out_buf, UINT32 buf_length);

WIDE_CHAR ruyi_unicode_wide_char_utf8(const char *str);

UINT32 ruyi_unicode_bytes(WIDE_CHAR c, BYTE *buf, UINT32 buf_length);

typedef struct {
    WIDE_CHAR *data;
    UINT32 length;
    UINT32 capacity;
} ruyi_unicode_string;

typedef struct {
    char *str;
    UINT32 length;
    UINT32 capacity;
} ruyi_bytes_string;

ruyi_unicode_string * ruyi_unicode_string_init_with_capacity(UINT32 capacity);

ruyi_unicode_string * ruyi_unicode_string_init_from_utf8(const char* src, UINT32 len);

ruyi_unicode_string * ruyi_unicode_string_copy_from(const ruyi_unicode_string * src);

void ruyi_unicode_string_append(ruyi_unicode_string *unicode_str, WIDE_CHAR *data, UINT32 len);

void ruyi_unicode_string_append_wide_char(ruyi_unicode_string *unicode_str, WIDE_CHAR c);

void ruyi_unicode_string_append_utf8(ruyi_unicode_string *unicode_str, const char* src, UINT32 len);

UINT32 ruyi_unicode_string_length(const ruyi_unicode_string *unicode_str);

WIDE_CHAR ruyi_unicode_string_at(const ruyi_unicode_string *unicode_str, UINT32 index);

void ruyi_unicode_string_set(ruyi_unicode_string *unicode_str, UINT32 index, WIDE_CHAR c);

void ruyi_unicode_string_destroy(ruyi_unicode_string* s);

ruyi_bytes_string* ruyi_unicode_string_decode_utf8(const ruyi_unicode_string *unicode_str);

void ruyi_unicode_bytes_string_destroy(ruyi_bytes_string* s);

BOOL ruyi_unicode_string_equals(const ruyi_unicode_string *unicode_str, const ruyi_unicode_string *unicode_str_other);

#endif /* ruyi_unicode_h */

