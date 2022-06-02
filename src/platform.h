#ifndef PLATFORM_H
#define PLATFORM_H

f32 get_time();

void *reserve_mem(u64 size);
void free_mem(void *mem, u64 size);

b32 is_file(struct str8 path);
b32 is_dir(struct str8 path);

struct str8 get_file_abspath(struct memory_arena *allocator, struct str8 file_path);
struct str8list get_dir_list_ext(struct memory_arena *allocator, struct str8 dir, struct str8 ext);

#endif
