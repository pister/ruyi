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
UINT32 ruyi_unicode_encode_utf8(const WIDE_CHAR* src_utf8, UINT32 src_len, UINT32 *src_used_count, BYTE *out_buf, UINT32 buf_length);

#endif /* ruyi_unicode_h */

