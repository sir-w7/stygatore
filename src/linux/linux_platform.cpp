#include <time.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <fcntl.h>

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "../platform.h"

styx_function f64
get_time()
{
	struct timespec time_spec_thing = {0};
	clock_gettime(CLOCK_REALTIME, &time_spec_thing);
	float seconds_elapsed = ((f64)time_spec_thing.tv_nsec / (f64)1e9);
	return seconds_elapsed * 1000.0f;
}

styx_inline void *
reserve_mem(u64 size)
{
	return mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

styx_inline void 
free_mem(void *mem, u64 size)
{
    munmap(mem, size);
}

styx_inline b32 
is_file(Str8 path)
{
	struct stat path_stat = {0};
	stat(path.str, &path_stat);
	return S_ISREG(path_stat.st_mode);
}

styx_inline b32 
is_dir(Str8 path)
{
	struct stat path_stat = {0};
	stat(path.str, &path_stat);
	return S_ISDIR(path_stat.st_mode);
}

styx_function Str8 
get_file_abspath(MemoryArena *allocator, Str8 file_path)
{
	static char buffer[256];
	realpath(file_path.str, buffer);
	return push_str8_copy(allocator, str8_from_cstr(buffer));
}

styx_function Str8List 
get_dir_list_ext(MemoryArena *allocator, 
                 Str8 dir_path, Str8 ext)
{
	Str8List dir_file_list = {0};
    
	auto dir = opendir(dir_path.str);	
	if (dir == NULL) {
		fprintln(stderr, "Not a directory. opendir failed.");
		return dir_file_list;
	}
	defer { closedir(dir); };
	    
	struct dirent *dir_entry = NULL;
	while ((dir_entry = readdir(dir)) != NULL) {
		Str8 filename = str8_from_cstr(dir_entry->d_name);
		if (dir_entry->d_type == DT_REG && str8_compare(file_ext(filename), ext)) {
			auto file_rel = push_str8_concat(allocator, dir_path, str8_lit("/"));
			file_rel = push_str8_concat(allocator, file_rel, filename);            
			str8list_push(&dir_file_list, allocator, file_rel);
		}
	}
    
	return dir_file_list;
}
