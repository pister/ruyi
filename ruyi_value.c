//
//  ruyi_value.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/15.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_value.h"

ruyi_value ruyi_value_uint64(UINT64 value) {
    ruyi_value v;
    v.type = Ruyi_value_type_uint64;
    v.data.uint64_value = value;
    return v;
}

ruyi_value ruyi_value_int64(INT64 value) {
    ruyi_value v;
    v.type = Ruyi_value_type_int64;
    v.data.int64_value = value;
    return v;
}
ruyi_value ruyi_value_uint32(UINT32 value) {
    ruyi_value v;
    v.type = Ruyi_value_type_uint32;
    // fill 0 for the other bits
    v.data.uint64_value = 0;
    v.data.uint32_value = value;
    return v;
}
ruyi_value ruyi_value_int32(INT32 value) {
    ruyi_value v;
    v.type = Ruyi_value_type_int32;
    // fill 0 for the other bits
    v.data.uint64_value = 0;
    v.data.int32_value = value;
    return v;
}
ruyi_value ruyi_value_uint16(UINT16 value) {
    ruyi_value v;
    v.type = Ruyi_value_type_uint16;
    // fill 0 for the other bits
    v.data.uint64_value = 0;
    v.data.uint16_value = value;
    return v;
}
ruyi_value ruyi_value_int16(INT16 value) {
    ruyi_value v;
    v.type = Ruyi_value_type_int16;
    // fill 0 for the other bits
    v.data.uint64_value = 0;
    v.data.int16_value = value;
    return v;
}
ruyi_value ruyi_value_uint8(UINT8 value) {
    ruyi_value v;
    v.type = Ruyi_value_type_uint8;
    // fill 0 for the other bits
    v.data.uint64_value = 0;
    v.data.uint8_value = value;
    return v;
}
ruyi_value ruyi_value_int8(INT8 value) {
    ruyi_value v;
    v.type = Ruyi_value_type_int8;
    // fill 0 for the other bits
    v.data.uint64_value = 0;
    v.data.int8_value = value;
    return v;
}
ruyi_value ruyi_value_ptr(void* ptr) {
    ruyi_value v;
    v.type = Ruyi_value_type_ptr;
    // fill 0 for the other bits on 32-bit env
    v.data.uint64_value = 0;
    v.data.ptr = ptr;
    return v;
}

BOOL ruyi_value_equals(ruyi_value v1, ruyi_value v2) {
    if (v1.type != v2.type) {
        return FALSE;
    }
    switch (v1.type) {
        case Ruyi_value_type_uint64:
            return v1.data.uint64_value == v2.data.uint64_value;
        case Ruyi_value_type_int64:
            return v1.data.int64_value == v2.data.int64_value;
        case Ruyi_value_type_uint32:
            return v1.data.uint32_value == v2.data.uint32_value;
        case Ruyi_value_type_int32:
            return v1.data.int32_value == v2.data.int32_value;
        case Ruyi_value_type_uint16:
            return v1.data.uint16_value == v2.data.uint16_value;
        case Ruyi_value_type_int16:
            return v1.data.int16_value == v2.data.int16_value;
        case Ruyi_value_type_uint8:
            return v1.data.uint8_value == v2.data.uint8_value;
        case Ruyi_value_type_int8:
            return v1.data.int8_value == v2.data.int8_value;
        case Ruyi_value_type_ptr:
            return v1.data.ptr == v2.data.ptr;
        default:
            break;
    }
    return FALSE;
}
