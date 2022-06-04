#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../common.h"
#include "../platform.h"

f64 get_time()
{
    static LARGE_INTEGER performance_frequency = {0};
    if(performance_frequency.QuadPart == 0) 
        QueryPerformanceFrequency(&performance_frequency);
    
    LARGE_INTEGER time_int;
    QueryPerformanceCounter(&time_int);
    return (f64)time_int.QuadPart / (f64)performance_frequency.QuadPart;
}

void *reserve_mem(u64 size) 
{
    return VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void free_mem(void *mem, u64 size)
{
    VirtualFree(mem, 0, MEM_RELEASE);
}

b32 is_dir(struct str8 path)
{
    DWORD file_attr = GetFileAttributesA(path.str);
    return (file_attr != INVALID_FILE_ATTRIBUTES &&
            (file_attr & FILE_ATTRIBUTE_DIRECTORY));
}

b32 is_file(struct str8 path)
{
    DWORD file_attr = GetFileAttributesA(path.str);
    return file_attr == FILE_ATTRIBUTE_NORMAL;
}

struct str8 
get_file_abspath(struct memory_arena *allocator, struct str8 file_path)
{
    static char buffer[256];
    u32 len = GetFullPathNameA(file_path.str, 256, buffer, NULL);
    
    // NOTE(sir->w7): Change to Unix-style paths (the real standard).
    for (int i = 0; i < len; ++i) {
        if (buffer[i] == '\\') {
            buffer[i] = '/';
        }
    }
    
    return push_str8_copy(allocator, str8_from_cstr(buffer));
}

struct str8list 
get_dir_list_ext(struct memory_arena *allocator, 
                 struct str8 dir_path, struct str8 ext)
{
	struct str8list dir_file_list = {0};
    
    WIN32_FIND_DATAA find_data = {0};
    HANDLE search_handle = {0};
    
    {
        // NOTE(sir->w7): Win32 API is pretty dumb, so we need to rework the string while keeping compatibility with Linux.
        
        struct temp_arena scratch = begin_temp_arena(allocator);
        struct str8 dir_path_hacked = {0};
        if (dir_path.str[dir_path.len - 1] == '\\' ||
            dir_path.str[dir_path.len - 1] == '/') {
            dir_path_hacked = push_str8_concat(allocator, dir_path, str8_lit("*"));
        } else {
            dir_path_hacked = push_str8_concat(allocator, dir_path, str8_lit("/*"));
        }
        search_handle = FindFirstFileA(dir_path_hacked.str, &find_data);
    }
    
    if (search_handle == INVALID_HANDLE_VALUE) {
        fprintln(stderr, "Not a directory. FindFirstFileA failed.");
		return dir_file_list;
    }
    
    do {
        struct str8 filename = str8_from_cstr(find_data.cFileName);
        
        // TODO(sir->w7): Test if it is actually a file because, just because it isn't a directory, it doesn't mean that it's a file.
		if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			str8_compare(file_ext(filename), ext)) {
			// TODO(sir->w7): For loop-based defer macro to simplify this interface.
			struct temp_arena scratch = begin_temp_arena(allocator); 
			
			struct str8 file_rel = push_str8_concat(allocator, dir_path, str8_lit("/"));
			file_rel = push_str8_concat(allocator, file_rel, filename);
            
			end_temp_arena(&scratch);
            
			struct str8 file_path = get_file_abspath(allocator, file_rel);
			str8list_push(&dir_file_list, allocator, file_path);
		}
    } while (FindNextFileA(search_handle, &find_data));
    
    FindClose(search_handle);
    
    return dir_file_list;
}
