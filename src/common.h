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

#define max(a, b) (a > b ? a : b)
#define min(a, b) (a > b ? b : a)

//-------------------------------Memory-------------------------------
#define DEF_ALIGN (2 * sizeof(void *))

struct memory_arena
{
	u8 *mem;
	u64 size;

	u64 prev_offset;
	u64 curr_offset;
};

uintptr_t align_forth(uintptr_t ptr, u32 align);

void memory_copy(void *dest, void *src, u64 size);
void memory_set(void *ptr, u32 value, u64 size);

struct memory_arena init_arena(u64 size);
void free_arena(struct memory_arena *arena);

void arena_reset(struct memory_arena *arena);
void *arena_push_align(struct memory_arena *arena, u64 size, u32 align);

struct temp_arena { 
	struct memory_arena *parent_arena;
	u64 prev_offset;
	u64 curr_offset;
};

struct temp_arena begin_temp_arena(struct memory_arena *arena);
void end_temp_arena(struct temp_arena *temp_arena);

#define arena_push(arena, size) arena_push_align(arena, size, DEF_ALIGN)
#define arena_push_array(arena, size, count) arena_push(arena, size * count)
#define arena_push_struct(arena, structure) (structure *)arena_push(arena, sizeof(structure))

#define temp_arena_push_align(temp, size, align) arena_push_align(temp->parent_arena, size, align)
#define temp_arena_push(temp, size) arena_push(temp->parent_arena, size)

//-------------------------------String-------------------------------
struct str8
{
	char *str;
	u64 len;
};

u64 cstr_len(char *cstr);

struct str8 push_str8(struct memory_arena *allocator, u8 *cstr, u64 len);
struct str8 push_str8_copy(struct memory_arena *allocator, struct str8 str);
struct str8 push_str8_concat(struct memory_arena *allocator, struct str8 init, struct str8 add);

// Returns true if the strings are the same, false if they are different.
b32 str8_compare(struct str8 str1, struct str8 str2);

struct str8node
{
	struct str8 data;
	struct str8node *next;
};

struct str8list
{
	struct str8node *head;
	struct str8node *tail;
};

void str8list_push(struct str8list *list, struct memory_arena *allocator, struct str8 str);

#define str8_lit(string) ((struct str8){.str = string, .len = sizeof(string) - 1})
#define str8_exp(string) string.len ? (int)string.len : 4, string.len ? string.str : "null"

#define str8_from_cstr(cstr) ((struct str8){.str = cstr, .len = cstr_len(cstr)})
#define str8_is_nil(string) (string.len == 0)

// File and string utilities.
struct str8 file_working_dir(struct str8 filename);
struct str8 file_base_name(struct str8 filename);
struct str8 file_ext(struct str8 filename);
struct str8 read_file(struct memory_arena *allocator, struct str8 filename);
struct str8list arg_list(struct memory_arena *allocator, int argc, char **argv);

// djb2 hash function for string hashing.
u64 get_hash(struct str8 str);

#endif
