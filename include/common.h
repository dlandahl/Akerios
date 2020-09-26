
#pragma once

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;

typedef i32      size;

#define size_of sizeof
#define if_not(cond) if (!(cond))
#define cast(Type, expression) ((Type) (expression))
#define nullptr 0
#define internal static
#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a > b) ? a : b)

typedef enum {
    false, true
} bool;

struct u64 {
    u32 lower, upper;
} __attribute__((packed));
