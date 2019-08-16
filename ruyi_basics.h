//
//  ruyi_types.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/15.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_types_h
#define ruyi_types_h

#include <inttypes.h>
#include <stdint.h>
#include <assert.h>

#if defined(_WIN32) || defined( __CYGWIN__)
#define FILE_PATH_SEP '\\'
#else
#define FILE_PATH_SEP '/'
#endif

/*
#ifdef WIN32
typedef __int64 INT64;
typedef unsigned __int64 UINT64;
#else
typedef long long INT64;
typedef unsigned long long UINT64;
#endif
*/

#ifndef UINT64
typedef uint64_t UINT64;
#endif

#ifndef INT64
typedef int64_t INT64;
#endif

#ifndef UINT32
typedef uint32_t UINT32;
#endif

#ifndef INT32
typedef int32_t INT32;
#endif

#ifndef UINT16
typedef uint16_t UINT16;
#endif

#ifndef UINT16
typedef int16_t INT16;
#endif

#ifndef UINT16
typedef uint16_t UINT16;
#endif

#ifndef INT16
typedef int16_t INT16;
#endif

#ifndef UINT8
typedef uint8_t UINT8;
#endif

#ifndef INT8
typedef int8_t INT8;
#endif

#ifndef BYTE
typedef UINT8 BYTE;
#endif

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif /* ruyi_types_h */
