#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "common.h"
#include "win32/win32_platform.cpp"

styx_inline b32
is_power_of_two(uintptr_t x) 
{
	return (x & (x - 1)) == 0;
}

styx_function uintptr_t 
align_forth(uintptr_t ptr, u32 align)
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
styx_function void 
memory_copy(void *dest, void *src, u64 size)
{
	char *dest_ptr = (char *)dest;
	char *src_ptr = (char *)src;
	
	for (u64 i = 0; i < size; ++i) {
		*dest_ptr++ = *src_ptr++;
	}
}

styx_function void
memory_set(void *ptr, u32 value, u64 size)
{
	char *dest_ptr = (char *)ptr;
	for (u64 i = 0; i < size; ++i) {
		dest_ptr[i] = (char)value;
	}
}

styx_function MemoryArena 
init_arena(u64 size)
{
	assert(size);
	
	MemoryArena arena = {0};
	arena.mem = (u8 *)reserve_mem(size);
	arena.size = size;
	
	return arena;
}

styx_function void 
free_arena(MemoryArena *arena)
{
	free_mem(arena->mem, arena->size);
	*arena = MemoryArena{};
}

styx_function void
arena_reset(MemoryArena *arena)
{
	if (arena == NULL) return;
	
	arena->curr_offset = 0;
}

styx_function void *
arena_push_align(MemoryArena *arena, u64 size, u32 align)
{
	uintptr_t curr_ptr = (uintptr_t)arena->mem + (uintptr_t)arena->curr_offset;
	uintptr_t offset = align_forth(curr_ptr, align);
	offset -= (uintptr_t)arena->mem;
	
	assert(offset + size <= arena->size);
	void *ptr = &arena->mem[offset];
	arena->curr_offset = offset + size;
	
	memory_set(ptr, 0, size);
	return ptr;
}

styx_function void *
arena_push_pack(MemoryArena *arena, u64 size)
{
    assert(arena->curr_offset + size <= arena->size);
	void *ptr = &arena->mem[arena->curr_offset];
    
	arena->curr_offset += size;
	
	memory_set(ptr, 0, size);
    
    return ptr;
}

//- NOTE(sir->w): Temp arena function definitions.
styx_function TempArena
begin_temp_arena(MemoryArena *arena)
{
	return TempArena{arena, arena->curr_offset};
}

// NOTE(sir->w): This is assuming no allocations is made on the main arena,
// which is a pretty far-fetched supposition. 
styx_function void 
end_temp_arena(TempArena *temp_arena)
{
	temp_arena->parent_arena->curr_offset = temp_arena->curr_offset;
}

//-------------------------------String-------------------------------
styx_function u64 
cstr_len(char *cstr)
{
	u64 len = 0;
	while (cstr[len++] != '\0');
	return len - 1;
}

styx_function Str8
push_str8(MemoryArena *allocator, u8 *cstr, u64 len)
{
	Str8 str = {0};
	str.len = len;
	str.str = (char *)arena_push(allocator, len + 1);
	
	memory_copy(str.str, cstr, len + 1);
	
	return str;
}

styx_function Str8 
push_str8_copy(MemoryArena *allocator, Str8 str)
{
	Str8 new_str = {0};
	new_str.len = str.len;
	new_str.str = (char *)arena_push(allocator, new_str.len);
	memory_copy(new_str.str, str.str, new_str.len);
	
	return new_str;
}

styx_function Str8 
push_str8_concat(MemoryArena *allocator, 
                 Str8 init, Str8 add)
{
	Str8 new_str = {0};
	
	new_str.len = init.len + add.len; 
	new_str.str = (char *)arena_push_array(allocator, sizeof(u8), new_str.len);
	memory_copy(new_str.str, init.str, init.len);
	memory_copy(new_str.str + init.len, add.str, add.len);
	new_str.str[new_str.len] = '\0';
	
	return new_str;
}

styx_function b32 
str8_compare(Str8 str1, Str8 str2)
{
	if (str1.len != str2.len) 
		return FALSE;
	
	for (u64 i = 0; i < str1.len; ++i) {
		if (str1.str[i] != str2.str[i])
			return FALSE;
	}
	return TRUE;
}

// If str is already dynamically allocated, this still creates another copy of 
// str. Perhaps we can add a flag to str to specify if it's a stack literal vs. 
// a dynamically allocated string to save memory.
styx_function void 
str8list_push(Str8List *list, 
              MemoryArena *allocator, Str8 str) 
{
	if (list->head == NULL) {
		list->head = arena_push_struct(allocator, Str8Node);
		list->head->data = push_str8_copy(allocator, str);
		
		list->tail = list->head;
		return;
	}
	
	Str8Node *new_node = arena_push_struct(allocator, Str8Node);
	new_node->data = push_str8_copy(allocator, str);
	
	list->tail->next = new_node;
	list->tail = new_node;
}

// File and string utilities.
styx_function Str8
file_working_dir(Str8 filename)
{
	Str8 working_dir = {filename.str};
	for (int i = static_cast<int>(filename.len - 1); i >= 0; --i) {
		if (filename.str[i] == '/') {
			working_dir.len = static_cast<u64>(i + 1);
			break;
		}
	}
	
	return working_dir;
}

styx_function Str8
file_base_name(Str8 filename)
{
	Str8 base_name = {0};
	u64 offset = 0;
	
	for (int i = static_cast<int>(filename.len - 1); i >= 0; --i) {
		if (filename.str[i] == '/') {
			offset = static_cast<u64>(i + 1);
			base_name.str = filename.str + offset;
			break;
		}
	}
	
	if (offset == 0) base_name.str = filename.str;
	
	for (u64 i = offset; i < filename.len; ++i) {
		if (filename.str[i] == '.') {
			base_name.len = i - offset;
			break;
		}
	}
	
	return base_name;
}

styx_function Str8
file_ext(Str8 filename)
{
	Str8 ext = {0};
	for (int i = static_cast<int>(filename.len - 1); i >= 0; --i) {
		if (filename.str[i] == '.') {
			ext.str = filename.str + i + 1;
			ext.len = filename.len - i - 1;
			break;
		}
	}
	return ext;
}

styx_function Str8 
read_file(MemoryArena *allocator, Str8 filename)
{
	Str8 file_data = {0};
	FILE *file = {};
    fopen_s(&file, filename.str, "r");
	if (!file) {
		fprintln(stderr, "Failed to open file.");
	}
	
	u64 file_size = 0;
	fseek(file, 0, SEEK_END);
	file_size = (u64)ftell(file);
	fseek(file, 0, SEEK_SET);
	
	file_data.str = (char *)arena_push(allocator, file_size + 1);
	file_data.len = file_size;
	
	fread(file_data.str, file_size, 1, file);
	file_data.str[file_size] = '\0';
	
	return file_data;
}

styx_function Str8List 
arg_list(MemoryArena *allocator, 
         int argc, char **argv)
{
	Str8List args = {0};
	
	// Starts at 1 to skip the command.
	for (int i = 1; i < argc; ++i) {
		str8list_push(&args, allocator, str8_from_cstr(argv[i]));
	}
	
	return args;
}

// djb2 hash function for string hashing.
styx_function u64
djb2_hash(Str8 str)
{
	u64 hash = 5381;
	
    for (u64 i = 0; i < str.len; ++i) {
		hash = ((hash << 5) + hash) + str.str[i];
	}
	return hash;
}