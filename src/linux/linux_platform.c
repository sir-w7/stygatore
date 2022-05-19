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

#include "../common.h"
#include "../platform.h"

f32 get_time()
{
	struct timespec time_spec_thing = {0};
	clock_gettime(CLOCK_REALTIME, &time_spec_thing);
	float seconds_elapsed = ((float)time_spec_thing.tv_nsec / (float)1e9);
	return seconds_elapsed;
}

void *reserve_mem(u64 size)
{
	return mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

void free_mem(void *mem, u64 size)
{
    munmap(mem, size);
}

b32 is_file(struct str8 path)
{
	struct stat path_stat = {0};
	stat(path.str, &path_stat);
	return S_ISREG(path_stat.st_mode);
}

b32 is_dir(struct str8 path)
{
	struct stat path_stat = {0};
	stat(path.str, &path_stat);
	return S_ISDIR(path_stat.st_mode);
}

struct str8list 
get_dir_list_ext(struct memory_arena *allocator, 
		 struct str8 dir_path, struct str8 ext)
{
	struct str8list dir_file_list = {0};
	char buffer[256];

	DIR *dir = opendir(dir_path.str);	
	if (dir == NULL) {
		fprintln(stderr, "Not a directory. opendir failed.");
		return dir_file_list;
	}

	struct dirent *dir_entry = NULL;
	while ((dir_entry = readdir(dir)) != NULL) {
		struct str8 filename = str8_from_cstr(dir_entry->d_name);
		if (dir_entry->d_type == DT_REG &&
			str8_compare(file_ext(filename), str8_lit("styxgen"))) {
			// For loop-based defer macro to simplify this interface.
			struct temp_arena scratch = begin_temp_arena(allocator); 
			
			struct str8 file_rel = push_str8_concat(allocator, dir_path, str8_lit("/"));
			file_rel = push_str8_concat(allocator, file_rel, filename);
			realpath(file_rel.str, buffer);

			end_temp_arena(&scratch);

			struct str8 file_path = push_str8_copy(allocator, str8_from_cstr(buffer));
			str8list_push(&dir_file_list, allocator, file_path);
		}
	}
	closedir(dir);

	return dir_file_list;
}
