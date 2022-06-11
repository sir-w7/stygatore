#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "common.h"
#include "platform.h"

static inline b32
is_power_of_two(uintptr_t x) 
{
	return (x & (x - 1)) == 0;
}

uintptr_t align_forth(uintptr_t ptr, u32 align)
{
	assert(is_power_of_two(align));
	uintptr_t p = ptr;
	uintptr_t a = (uintptr_t)align;

	// NOTE(sir->w): Basically p % a. No clue how this works.
	uintptr_t modulo = p & (a - 1);
	if (modulo != 0) {
		p += a - modulo;
	}

	return p;
}

// Basic memory function implementations that we need so we don't
// need to import all of string.h.
void memory_copy(void *dest, void *src, u64 size)
{
	char *dest_ptr = (char *)dest;
	char *src_ptr = (char *)src;

	for (u64 i = 0; i < size; ++i) {
		*dest_ptr++ = *src_ptr++;
	}
}

void memory_set(void *ptr, u32 value, u64 size)
{
	char *dest_ptr = (char *)ptr;
	for (int i = 0; i < size; ++i) {
		dest_ptr[i] = value;
	}
}

struct memory_arena 
init_arena(u64 size)
{
	assert(size);

	struct memory_arena arena = {0};
	arena.mem = reserve_mem(size);
	arena.size = size;

	return arena;
}

void free_arena(struct memory_arena *arena)
{
	free_mem(arena->mem, arena->size);
	*arena = (struct memory_arena){0};
}

void arena_reset(struct memory_arena *arena)
{
	if (arena == NULL) return;

	arena->prev_offset = 0;
	arena->curr_offset = 0;
}

void *arena_push_align(struct memory_arena *arena, u64 size, u32 align)
{
	uintptr_t curr_ptr = (uintptr_t)arena->mem + (uintptr_t)arena->curr_offset;
	uintptr_t offset = align_forth(curr_ptr, align);
	offset -= (uintptr_t)arena->mem;

	assert(offset + size <= arena->size);
	void *ptr = &arena->mem[offset];
	arena->prev_offset = offset;
	arena->curr_offset = offset + size;

	memory_set(ptr, 0, size);
	return ptr;
}

//- NOTE(sir->w): Temp arena function definitions.
struct temp_arena
begin_temp_arena(struct memory_arena *arena)
{
	struct temp_arena temp = {
		.parent_arena = arena,

		.prev_offset = arena->prev_offset,
		.curr_offset = arena->curr_offset,
	};

	return temp;
}

// NOTE(sir->w): This is assuming no allocations is made on the main arena,
// which is a pretty far-fetched supposition. 
void end_temp_arena(struct temp_arena *temp_arena)
{
	temp_arena->parent_arena->prev_offset = temp_arena->prev_offset;
	temp_arena->parent_arena->curr_offset = temp_arena->curr_offset;
}

//-------------------------------String-------------------------------
u64 cstr_len(char *cstr)
{
	u64 len = 0;
	while (cstr[len++] != '\0');
	return len - 1;
}

struct str8
push_str8(struct memory_arena *allocator, u8 *cstr, u64 len)
{
	struct str8 str = {0};
	str.len = len;
	str.str = (char *)arena_push(allocator, len + 1);

	memory_copy(str.str, cstr, len + 1);

	return str;
}

struct str8 
push_str8_copy(struct memory_arena *allocator, struct str8 str)
{
	struct str8 new_str = {0};
	new_str.len = str.len;
	new_str.str = (char *)arena_push(allocator, new_str.len);
	memory_copy(new_str.str, str.str, new_str.len);

	return new_str;
}

struct str8 
push_str8_concat(struct memory_arena *allocator, 
                 struct str8 init, struct str8 add)
{
	struct str8 new_str = {0};

	new_str.len = init.len + add.len; 
	new_str.str = (char *)arena_push_array(allocator, sizeof(u8), new_str.len);
	memory_copy(new_str.str, init.str, init.len);
	memory_copy(new_str.str + init.len, add.str, add.len);
	new_str.str[new_str.len] = '\0';

	return new_str;
}

b32 str8_compare(struct str8 str1, struct str8 str2)
{
	if (str1.len != str2.len) 
		return FALSE;

	for (int i = 0; i < str1.len; ++i) {
		if (str1.str[i] != str2.str[i])
			return FALSE;
	}
	return TRUE;
}

// If str is already dynamically allocated, this still creates another copy of 
// str. Perhaps we can add a flag to str to specify if it's a stack literal vs. 
// a dynamically allocated string to save memory.
void str8list_push(struct str8list *list, 
                   struct memory_arena *allocator, struct str8 str) 
{
	if (list->head == NULL) {
		list->head = arena_push_struct(allocator, struct str8node);
		list->head->data = push_str8_copy(allocator, str);

		list->tail = list->head;
		return;
	}

	struct str8node *new_node = arena_push_struct(allocator, struct str8node);
	new_node->data = push_str8_copy(allocator, str);

	list->tail->next = new_node;
	list->tail = new_node;
}

// File and string utilities.
struct str8
file_working_dir(struct str8 filename)
{
	struct str8 working_dir = {
		.str = filename.str,
	};
	for (int i = filename.len - 1; i >= 0; --i) {
		if (filename.str[i] == '/') {
			working_dir.len = i + 1;
			break;
		}
	}

	return working_dir;
}

struct str8
file_base_name(struct str8 filename)
{
	struct str8 base_name = {0};
	int offset = 0;

	for (int i = filename.len - 1; i >= 0; --i) {
		if (filename.str[i] == '/') {
			offset = i + 1;
			base_name.str = filename.str + offset;
			break;
		}
	}

	if (offset == 0) base_name.str = filename.str;

	for (int i = offset; i < filename.len; ++i) {
		if (filename.str[i] == '.') {
			base_name.len = i - offset;
			break;
		}
	}

	return base_name;
}

struct str8
file_ext(struct str8 filename)
{
	struct str8 ext = {0};
	for (int i = filename.len - 1; i >= 0; --i) {
		if (filename.str[i] == '.') {
			ext.str = filename.str + i + 1;
			ext.len = filename.len - i - 1;
			break;
		}
	}
	return ext;
}

struct str8 
read_file(struct memory_arena *allocator, struct str8 filename)
{
	struct str8 file_data = {0};
	FILE *file = fopen(filename.str, "r");
	if (!file) {
		fprintln(stderr, "Failed to open file.");
	}

	u64 file_size = 0;
	fseek(file, 0, SEEK_END);
	file_size = (u64)ftell(file);
	fseek(file, 0, SEEK_SET);

	file_data.str = arena_push(allocator, file_size + 1);
	file_data.len = file_size;

	fread(file_data.str, file_size, 1, file);
	file_data.str[file_size] = '\0';

	return file_data;
}

struct str8list 
arg_list(struct memory_arena *allocator, 
         int argc, char **argv)
{
	struct str8list args = {0};

	// Starts at 1 to skip the command.
	for (int i = 1; i < argc; ++i) {
		str8list_push(&args, allocator, str8_from_cstr(argv[i]));
	}

	return args;
}

// djb2 hash function for string hashing.
u64 djb2_hash(struct str8 str)
{
	u64 hash = 5381;
	i32 c = 0;

	while ((c = *str.str++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}


