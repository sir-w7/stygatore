#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../platform.h"

styx_function f64
get_time()
{
	static LARGE_INTEGER performance_frequency = {};
	if(performance_frequency.QuadPart == 0) 
		QueryPerformanceFrequency(&performance_frequency);
	
	LARGE_INTEGER time_int;
	QueryPerformanceCounter(&time_int);
	return 1000.0f * ((f64)time_int.QuadPart / (f64)performance_frequency.QuadPart);
}

styx_inline void *
reserve_mem(u64 size) 
{
	return VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

styx_inline void 
free_mem(void *mem, u64 size)
{
	VirtualFree(mem, 0, MEM_RELEASE);
}

styx_inline b32 
is_dir(Str8 path)
{
	DWORD file_attr = GetFileAttributesA(path.str);
	return (file_attr != INVALID_FILE_ATTRIBUTES &&
			(file_attr & FILE_ATTRIBUTE_DIRECTORY));
}

styx_inline b32 
is_file(Str8 path)
{
	DWORD file_attr = GetFileAttributesA(path.str);
    
    // NOTE(sir->w7): If the function succeeds and returns a file attribute that isn't a directory, we assume it's a file.
	return (file_attr != INVALID_FILE_ATTRIBUTES &&
            !(file_attr & FILE_ATTRIBUTE_DIRECTORY));
}

styx_function Str8
get_file_unixpath(MemoryArena *allocator, Str8 file_path)
{
    // NOTE(sir->w7): Change to Unix-style paths (the real standard).
	for (u32 i = 0; i < file_path.len; ++i) {
		if (file_path.str[i] == '\\') {
			file_path.str[i] = '/';
		}
	}
    
    return file_path;
}

styx_function Str8 
get_file_abspath(MemoryArena *allocator, Str8 file_path)
{
	static char buffer[256];
	u32 len = GetFullPathNameA(file_path.str, 256, buffer, NULL);
	
	return get_file_unixpath(allocator, push_str8_copy(allocator, str8_from_cstr(buffer)));
}

styx_function Str8List 
get_dir_list_ext(MemoryArena *allocator, 
				 Str8 dir_path, Str8 ext)
{
	Str8List dir_file_list = {0};
	
	WIN32_FIND_DATAA find_data = {0};
	HANDLE search_handle = {0};
	
    // TODO(sir->w7): Is there any way to get a list of all files in the directory without hacking together a path?
	{
		// NOTE(sir->w7): Win32 API is pretty dumb, so we need to rework the string while keeping compatibility with Linux.
		TempArena scratch = begin_temp_arena(allocator);
        defer { end_temp_arena(&scratch); };
        
        Str8 dir_path_hacked = {0};
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
		Str8 filename = str8_from_cstr(find_data.cFileName);
		
		// TODO(sir->w7): Test if it is actually a file because, 
		// just because it isn't a directory, it doesn't mean that it's a file.
		if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
		    str8_compare(file_ext(filename), ext)) {
			Str8 file_rel{};
            file_rel = push_str8_concat(allocator, dir_path, str8_lit("/"));
            file_rel = push_str8_concat(allocator, file_rel, filename);
            
			str8list_push(&dir_file_list, allocator, get_file_unixpath(allocator, file_rel));
		}
	} while (FindNextFileA(search_handle, &find_data));
	
	FindClose(search_handle);
	
	return dir_file_list;
}
