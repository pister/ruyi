//
//  ruyi_bytes.h
//  ruyi
//
//  Created by Songli Huang on 2020/2/16.
//  Copyright © 2020 Songli Huang. All rights reserved.
//

#ifndef ruyi_bytes_h
#define ruyi_bytes_h

#include "ruyi_basics.h"

UINT16 ruyi_bytes_to_uint16_big_endian(const void *buf);

UINT16 ruyi_bytes_to_uint16_little_endian(const void *buf);

UINT32 ruyi_bytes_to_uint32_big_endian(const void *buf);

UINT32 ruyi_bytes_to_uint32_little_endian(const void *buf);

UINT64 ruyi_bytes_to_uint64_big_endian(const void *buf);

UINT64 ruyi_bytes_to_uint64_little_endian(const void *buf);

void ruyi_bytes_from_uint16_big_endian(void *buf, UINT16 value);

void ruyi_bytes_from_uint16_little_endian(void *buf, UINT16 value);

void ruyi_bytes_from_uint32_big_endian(void *buf, UINT32 value);

void ruyi_bytes_from_uint32_little_endian(void *buf, UINT32 value);

void ruyi_bytes_from_uint64_big_endian(void *buf, UINT64 value);

void ruyi_bytes_from_uint64_little_endian(void *buf, UINT64 value);

#endif /* ruyi_bytes_h */
