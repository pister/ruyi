//
//  ruyi_bytes.c
//  ruyi
//
//  Created by Songli Huang on 2020/2/16.
//  Copyright © 2020 Songli Huang. All rights reserved.
//

#include "ruyi_bytes.h"
#include "ruyi_basics.h"

static const UINT16 ruyi_flag_value = 0x4321;
#define RUYI_LITTLE_ENDIAN() (0x21 == *(UINT8*)&ruyi_flag_value)
#define RUYI_BIG_ENDIAN() (0x43 == *(UINT8*)&ruyi_flag_value)

static UINT16 ruyi_swap_order_uint16(UINT16 v) {
    UINT16 b1 = 0xFF & v;
    UINT16 b2 = 0xFF & (v >> 8);
    return (b1 << 8)  + b2;
}

static UINT32 ruyi_swap_order_uint32(UINT32 v) {
    UINT32 b1 = 0xFF & v;
    UINT32 b2 = 0xFF & (v >> 8);
    UINT32 b3 = 0xFF & (v >> 16);
    UINT32 b4 = 0xFF & (v >> 24);
    return (b1 << 24)  + (b2 << 16) + (b3 << 8) + b4;
}

static UINT64 ruyi_swap_order_uint64(UINT64 v) {
    UINT64 b1 = 0xFF & v;
    UINT64 b2 = 0xFF & (v >> 8);
    UINT64 b3 = 0xFF & (v >> 16);
    UINT64 b4 = 0xFF & (v >> 24);
    UINT64 b5 = 0xFF & (v >> 32);
    UINT64 b6 = 0xFF & (v >> 40);
    UINT64 b7 = 0xFF & (v >> 48);
    UINT64 b8 = 0xFF & (v >> 56);
    return (b1 << 56)  + (b2 << 48) + (b3 << 40) + (b4 << 32) + (b5 << 24) + (b6 << 16) + (b7 << 8) + b8;
}

UINT16 ruyi_bytes_to_uint16_big_endian(const void *buf) {
    UINT16 v = *(UINT16*)buf;
    if (RUYI_BIG_ENDIAN()) {
        return v;
    } else {
        return ruyi_swap_order_uint16(v);
    }
}

UINT16 ruyi_bytes_to_uint16_little_endian(const void *buf) {
    UINT16 v = *(UINT16*)buf;
    if (RUYI_LITTLE_ENDIAN()) {
        return v;
    } else {
        return ruyi_swap_order_uint16(v);
    }
}


UINT32 ruyi_bytes_to_uint32_big_endian(const void *buf) {
    UINT32 v = *(UINT32*)buf;
    if (RUYI_BIG_ENDIAN()) {
        return v;
    } else {
        return ruyi_swap_order_uint32(v);
    }
}

UINT32 ruyi_bytes_to_uint32_little_endian(const void *buf) {
    UINT32 v = *(UINT32*)buf;
    if (RUYI_LITTLE_ENDIAN()) {
        return v;
    } else {
        return ruyi_swap_order_uint32(v);
    }
}


UINT64 ruyi_bytes_to_uint64_big_endian(const void *buf) {
    UINT64 v = *(UINT64*)buf;
    if (RUYI_BIG_ENDIAN()) {
        return v;
    } else {
        return ruyi_swap_order_uint64(v);
    }
}

UINT64 ruyi_bytes_to_uint64_little_endian(const void *buf) {
    UINT64 v = *(UINT64*)buf;
    if (RUYI_LITTLE_ENDIAN()) {
        return v;
    } else {
        return ruyi_swap_order_uint64(v);
    }
}

void ruyi_bytes_from_uint16_big_endian(void *buf, UINT16 value) {
    *(UINT16*)buf = ruyi_bytes_to_uint16_big_endian(&value);
}

void ruyi_bytes_from_uint16_little_endian(void *buf, UINT16 value) {
    *(UINT16*)buf = ruyi_bytes_to_uint16_little_endian(&value);
}

void ruyi_bytes_from_uint32_big_endian(void *buf, UINT32 value) {
    *(UINT32*)buf = ruyi_bytes_to_uint32_big_endian(&value);
}

void ruyi_bytes_from_uint32_little_endian(void *buf, UINT32 value) {
    *(UINT32*)buf = ruyi_bytes_to_uint32_little_endian(&value);
}

void ruyi_bytes_from_uint64_big_endian(void *buf, UINT64 value) {
    *(UINT64*)buf = ruyi_bytes_to_uint64_big_endian(&value);
}

void ruyi_bytes_from_uint64_little_endian(void *buf, UINT64 value) {
    *(UINT64*)buf = ruyi_bytes_to_uint64_little_endian(&value);
}
