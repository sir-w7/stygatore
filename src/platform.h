#ifndef PLATFORM_H
#define PLATFORM_H

styx_function f64 get_time();

styx_inline void *reserve_mem(u64 size);
styx_inline void free_mem(void *mem, u64 size);

styx_inline b32 is_file(Str8 path);
styx_inline b32 is_dir(Str8 path);

styx_function Str8 get_file_abspath(MemoryArena *allocator, Str8 file_path);
styx_function Str8List get_dir_list_ext(MemoryArena *allocator, Str8 dir, Str8 ext);

#endif
