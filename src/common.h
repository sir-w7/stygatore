#ifndef __LAYER_H_
#define __LAYER_H_

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

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

#define cast(variable_identifier, type) ((type)variable_identifier)

#define array_count(arr) (sizeof(arr) / sizeof(arr[0]))

#define _println(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define println(fmt, ...) _println(fmt, ##__VA_ARGS__)

#define _fprintln(stream, fmt, ...) fprintf(stream, fmt "\n", ##__VA_ARGS__)
#define fprintln(stream, fmt, ...) _fprintln(stream, fmt, ##__VA_ARGS__)

#define printnl() printf("\n")

//--------------------------------------------------------------------
//--------------------------Basic Utilities---------------------------
//--------------------------------------------------------------------

#ifndef max
#define max(a, b) (a > b ? a : b)
#endif

#ifndef min
#define min(a, b) (a > b ? b : a)
#endif 

#define defer_block(start, end) \
for (int _i_##__LINE__ = ((start), 0);  _i_##__LINE__ == 0; _i_##__LINE__ += 1, (end))

//-------------------------------Memory-------------------------------
#define DEF_ALIGN (2 * sizeof(void *))

typedef struct MemoryArena MemoryArena;
struct MemoryArena
{
	u8 *mem;
	u64 size;
    
	u64 prev_offset;
	u64 curr_offset;
};

uintptr_t align_forth(uintptr_t ptr, u32 align);

void memory_copy(void *dest, void *src, u64 size);
void memory_set(void *ptr, u32 value, u64 size);

MemoryArena init_arena(u64 size);
void free_arena(MemoryArena *arena);

void arena_reset(MemoryArena *arena);
void *arena_push_align(MemoryArena *arena, u64 size, u32 align);

typedef struct TempArena TempArena;
struct TempArena { 
	MemoryArena *parent_arena;
	u64 prev_offset;
	u64 curr_offset;
};

TempArena begin_temp_arena(MemoryArena *arena);
void end_temp_arena(TempArena *temp_arena);

#define arena_push(arena, size) arena_push_align(arena, size, DEF_ALIGN)
#define arena_push_array(arena, size, count) arena_push(arena, size * count)
#define arena_push_struct(arena, structure) (structure *)arena_push(arena, sizeof(structure))

#define temp_arena_push_align(temp, size, align) arena_push_align(temp->parent_arena, size, align)
#define temp_arena_push(temp, size) arena_push(temp->parent_arena, size)

//-------------------------------String-------------------------------
typedef struct Str8 Str8;
struct Str8
{
	char *str;
	u64 len;
};

u64 cstr_len(char *cstr);

Str8 push_str8(MemoryArena *allocator, u8 *cstr, u64 len);
Str8 push_str8_copy(MemoryArena *allocator, Str8 str);
Str8 push_str8_concat(MemoryArena *allocator, Str8 init, Str8 add);

// Returns true if the strings are the same, false if they are different.
b32 str8_compare(Str8 str1, Str8 str2);

typedef struct Str8Node Str8Node;
struct Str8Node
{
	Str8 data;
	Str8Node *next;
};

typedef struct Str8List Str8List;
struct Str8List
{
	Str8Node *head;
	Str8Node *tail;
};

void str8list_push(Str8List *list, MemoryArena *allocator, Str8 str);

#define str8_lit(string) ((Str8){.str = string, .len = sizeof(string) - 1})
#define str8_exp(string) string.len ? (int)string.len : 4, string.len ? string.str : "null"

#define str8_from_cstr(cstr) ((Str8){.str = cstr, .len = cstr_len(cstr)})
#define str8_is_nil(string) (string.len == 0)

// File and string utilities.
Str8 file_working_dir(Str8 filename);
Str8 file_base_name(Str8 filename);
Str8 file_ext(Str8 filename);
Str8 read_file(MemoryArena *allocator, Str8 filename);
Str8List arg_list(MemoryArena *allocator, int argc, char **argv);

// djb2 hash function for string hashing.
u64 djb2_hash(Str8 str);

#endif
