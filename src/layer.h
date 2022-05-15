#ifndef __LAYER_H_
#define __LAYER_H_

#include <stdint.h>
#include <stdio.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

typedef u8  b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

#define FALSE 0
#define TRUE  1

#define kilobytes(count) (1024ull * (u64)count)
#define megabytes(count) (1024ull * (u64)kilobytes(count))
#define gigabytes(count) (1024ull * (u64)megabytes(count))

#define void_check assert

#define cast(variable_identifier, type) ((type)variable_identifier)

#define _println(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define println(fmt, ...) _println(fmt, ##__VA_ARGS__)

#define _fprintln(stream, fmt, ...) fprintf(stream, fmt "\n", ##__VA_ARGS__)
#define fprintln(stream, fmt, ...) _fprintln(stream, fmt, ##__VA_ARGS__)

#define printnl() printf("\n")

#endif
