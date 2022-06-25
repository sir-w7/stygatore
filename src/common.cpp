#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "common.h"
#include "platform.h"

b32 is_power_of_two(uintptr_t x) 
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
	for (u64 i = 0; i < size; ++i) {
		dest_ptr[i] = (char)value;
	}
}

MemoryArena::MemoryArena(u64 size)
{
	assert(size);
	mem = static_cast<u8 *>(reserve_mem(size));
	this->size = size;
    offset = 0;
}

MemoryArena::~MemoryArena()
{
	free_mem(mem, size);
    // NOTE(sir->w): Should we zero out the struct as well?
}

void MemoryArena::reset()
{
    offset = 0;
}

void *MemoryArena::push_align(u64 size, u32 align)
{
	uintptr_t curr_ptr = (uintptr_t)mem + (uintptr_t)offset;
	uintptr_t offset = align_forth(curr_ptr, align);
	offset -= (uintptr_t)mem;
	
	assert(offset + size <= this->size);
	void *ptr = &mem[offset];
	this->offset = offset + size;
	
	memory_set(ptr, 0, size);

    return ptr;
}

void *MemoryArena::push_initialize_align(u64 size, void *init_data, u32 align)
{
	uintptr_t curr_ptr = (uintptr_t)mem + (uintptr_t)offset;
	uintptr_t offset = align_forth(curr_ptr, align);
	offset -= (uintptr_t)mem;
	
	assert(offset + size <= this->size);
	void *ptr = &mem[offset];
	this->offset = offset + size;
	
	memory_copy(ptr, init_data, size);

    return ptr;
}
void *MemoryArena::push_pack(u64 size)
{
    assert(offset + size <= this->size);
	void *ptr = &mem[offset];
	offset += size;
	
	memory_set(ptr, 0, size);
    
    return ptr;
}

//-------------------------------String-------------------------------
Str8::Str8(MemoryArena *allocator, char* cstr, u64 len)
{
	this->len = len;
	str = (char *)allocator->push(len + 1);
    memory_copy(str, cstr, len + 1);
}

Str8::Str8(MemoryArena *allocator, Str8 str)
{
    len = str.len;
    this->str = (char *)allocator->push(len);
    memory_copy(this->str, str.str, len);
}

u64 cstr_len(char *cstr)
{
	u64 len = 0;
	while (cstr[len++] != '\0');
	return len - 1;
}

Str8 push_str8_concat(MemoryArena *allocator, 
                 Str8 init, Str8 add)
{
	Str8 new_str;
	
	new_str.len = init.len + add.len; 
	new_str.str = (char *)allocator->push_array(sizeof(u8), new_str.len);
	memory_copy(new_str.str, init.str, init.len);
	memory_copy(new_str.str + init.len, add.str, add.len);
	new_str.str[new_str.len] = '\0';
	
	return new_str;
}

b32 str8_compare(Str8 str1, Str8 str2)
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
void Str8List::push(MemoryArena *allocator, Str8 str)
{
	count++;
	if (head == nullptr) {
		head = (Str8Node *)allocator->push(sizeof(Str8Node));
		head->data = Str8(allocator, str);

		tail = head;
		return;
	}

	auto new_node = (Str8Node *)allocator->push(sizeof(Str8Node));
	new_node->data = Str8(allocator, str);

	tail->next = new_node;
	tail = new_node;
}

// File and string utilities.
Str8 file_working_dir(Str8 filename)
{
	Str8 working_dir;
	working_dir.str = filename.str;
	for (int i = static_cast<int>(filename.len - 1); i >= 0; --i) {
		if (filename.str[i] == '/') {
			working_dir.len = static_cast<u64>(i + 1);
			break;
		}
	}
	
	return working_dir;
}

Str8 file_name(Str8 file_path)
{
    Str8 filename{};
    u32 offset = 0;
    
    for (int i = static_cast<int>(file_path.len - 1); i >= 0; --i) {
		if (file_path.str[i] == '/') {
			offset = static_cast<u64>(i + 1);
			filename.str = file_path.str + offset;
			break;
		}
	}
    
    filename.len = file_path.len - offset;
    
    return filename;
}

Str8 file_base_name(Str8 filename)
{
	Str8 base_name;
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

Str8 file_ext(Str8 filename)
{
	Str8 ext;
	for (int i = static_cast<int>(filename.len - 1); i >= 0; --i) {
		if (filename.str[i] == '.') {
			ext.str = filename.str + i + 1;
			ext.len = filename.len - i - 1;
			break;
		}
	}
	return ext;
}

Str8 read_file(MemoryArena *allocator, Str8 filename)
{
	Str8 file_data;
	auto file = fopen(filename.str, "r");
	if (file == NULL) {
		fprintln(stderr, "Failed to open file.");
		return file_data;
	}
	
	u64 file_size = 0;
	fseek(file, 0, SEEK_END);
	file_size = (u64)ftell(file);
	fseek(file, 0, SEEK_SET);
	
	file_data.str = static_cast<char *>(allocator->push(file_size + 1));
	file_data.len = file_size;
	
	fread(file_data.str, file_size, 1, file);
	file_data.str[file_size] = '\0';
	
	return file_data;
}

Str8List arg_list(MemoryArena *allocator, 
                  int argc, char **argv)
{
	Str8List args{};
	
	// Starts at 1 to skip the command.
	for (int i = 1; i < argc; ++i) {
		args.push(allocator, str8_from_cstr(argv[i]));
	}
	
	return args;
}

// djb2 hash function for string hashing.
u64 djb2_hash(Str8 str)
{
	u64 hash = 5381;
	
    for (u64 i = 0; i < str.len; ++i) {
		hash = ((hash << 5) + hash) + str.str[i];
	}
	return hash;
}
