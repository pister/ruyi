//
//  ruyi_value.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/15.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_value_h
#define ruyi_value_h

#include "ruyi_basics.h"

typedef enum {
    Ruyi_value_type_uint64,
    Ruyi_value_type_int64,
    Ruyi_value_type_uint32,
    Ruyi_value_type_int32,
    Ruyi_value_type_uint16,
    Ruyi_value_type_int16,
    Ruyi_value_type_uint8,
    Ruyi_value_type_int8,
    Ruyi_value_type_ptr,
    Ruyi_value_type_str,
    Ruyi_value_type_unicode_str,
} ruyi_value_type;

typedef struct {
    ruyi_value_type type;
    union {
        UINT64 uint64_value;
        INT64 int64_value;
        void * ptr;
        const char * str;
        const WIDE_CHAR * unicode_str;
        UINT32 uint32_value;
        INT32 int32_value;
        UINT16 uint16_value;
        INT16 int16_value;
        UINT16 uint8_value;
        INT16 int8_value;
    } data;
} ruyi_value;

ruyi_value ruyi_value_uint64(UINT64 value);
ruyi_value ruyi_value_int64(INT64 value);
ruyi_value ruyi_value_uint32(UINT32 value);
ruyi_value ruyi_value_int32(INT32 value);
ruyi_value ruyi_value_uint16(UINT16 value);
ruyi_value ruyi_value_int16(INT16 value);
ruyi_value ruyi_value_uint8(UINT8 value);
ruyi_value ruyi_value_int8(INT8 value);
ruyi_value ruyi_value_ptr(void *ptr);
ruyi_value ruyi_value_str(const char *str);
ruyi_value ruyi_value_unicode_str(const WIDE_CHAR *unicode_str);


BOOL ruyi_value_equals(ruyi_value v1, ruyi_value v2);
UINT32 ruyi_value_hashcode(ruyi_value value);

typedef int (*ruyi_value_comparator)(ruyi_value *v1, ruyi_value *v2);

#endif /* ruyi_value_h */
