#ifndef PLATFORM_H
#define PLATFORM_H

f64 get_time();

void *reserve_mem(u64 size);
void free_mem(void *mem, u64 size);

b32 is_file(Str8 path);
b32 is_dir(Str8 path);

Str8 get_file_abspath(MemoryArena *allocator, Str8 file_path);
Str8List get_dir_list_ext(MemoryArena *allocator, Str8 dir, Str8 ext);

#endif
