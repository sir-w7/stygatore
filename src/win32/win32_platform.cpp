#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../common.h"
#include "../platform.h"

struct ThreadPool
{
    ThreadContext thr_ctx;

    HANDLE thread;
    HANDLE semaphore;

    Str8 data[128];
    HandleFunc *jobs[128];

    u32 write_at = 0;
    u32 read_at = 0;

    u32 job_cnt = 0;

    bool running;
};

struct ThreadPools
{
    ThreadPool **pools;
    u32 cnt;
} pools;

static DWORD WINAPI
thread_proc(LPVOID param)
{
    auto thr_pool = static_cast<ThreadPool *>(param);
    thr_pool->running = true;
    
    while (thr_pool->running || thr_pool->job_cnt > 0) {
        WaitForSingleObject(thr_pool->semaphore, INFINITE);
        if (thr_pool->running == false && thr_pool->job_cnt == 0) {
            break;
        }

        thr_pool->jobs[thr_pool->read_at](&thr_pool->thr_ctx.allocator,
                                          &thr_pool->thr_ctx.temp_allocator,
                                          thr_pool->data[thr_pool->read_at]);
        thr_pool->read_at++;
        thr_pool->job_cnt--;
    }
    return 0;
}

static ThreadPool *
init_thread_pool(MemoryArena *lord_allocator)
{
    static u32 thr_idx = 0;
    ThreadPool *pool = (ThreadPool *)lord_allocator->push(sizeof(ThreadPool));
    
    pool->thr_ctx.thr_idx = thr_idx++;
    pool->thr_ctx.allocator = MemoryArena(lord_allocator, megabytes(128));
    pool->thr_ctx.temp_allocator = MemoryArena(lord_allocator, megabytes(256));

    pool->write_at = 0;
    pool->read_at = 0;
    pool->job_cnt = 0;
    
    pool->semaphore = CreateSemaphoreA(NULL, 0, 16, NULL);
    pool->thread = CreateThread(NULL, 0, thread_proc, pool, 0, 0);

    return pool;
}

void init_pools(MemoryArena *lord_allocator, u32 cnt)
{
    profile_def();
    pools.pools = (ThreadPool **)lord_allocator->push_array(sizeof(ThreadPool *), cnt);
    pools.cnt = cnt;

    for (int i = 0; i < cnt; ++i) {
        pools.pools[i] = init_thread_pool(lord_allocator);
    }
}

void queue_job(HandleFunc *func, Str8 data)
{
    // Let's pretend there's two jobs.
    static int idx = 0;
    if (idx > pools.cnt)
        idx = 0;

    auto pool = pools.pools[idx];
    pool->job_cnt++;

    pool->jobs[pool->write_at] = func;
    pool->data[pool->write_at] = data;
    pool->write_at++;

    idx++;
    
    ReleaseSemaphore(pool->semaphore, 1, NULL);
}

void wait_pools()
{
    profile_def();
    for (int i = 0; i < pools.cnt; ++i) {
        WaitForSingleObject(pools.pools[i]->thread, INFINITE);
        CloseHandle(pools.pools[i]->thread);
        CloseHandle(pools.pools[i]->semaphore);
    }
}

void send_kill_signals()
{
    profile_def();
    for (int i = 0; i < pools.cnt; ++i) {
        pools.pools[i]->running = false;
        ReleaseSemaphore(pools.pools[i]->semaphore, 1, NULL);
    }
}

f64 get_time()
{
    static LARGE_INTEGER performance_frequency = {};
    if(performance_frequency.QuadPart == 0) 
        QueryPerformanceFrequency(&performance_frequency);
	
    LARGE_INTEGER time_int;
    QueryPerformanceCounter(&time_int);
    return 1000.0f * ((f64)time_int.QuadPart / (f64)performance_frequency.QuadPart);
}

void *reserve_mem(u64 size) 
{ profile_def();
    return VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void free_mem(void *mem, u64 size)
{
    VirtualFree(mem, 0, MEM_RELEASE);
}

b32 is_dir(Str8 path)
{
    DWORD file_attr = GetFileAttributesA(path.str);
    return (file_attr != INVALID_FILE_ATTRIBUTES &&
            (file_attr & FILE_ATTRIBUTE_DIRECTORY));
}

b32 is_file(Str8 path)
{
    DWORD file_attr = GetFileAttributesA(path.str);
    
    // NOTE(sir->w7): If the function succeeds and returns a file attribute that isn't a directory, we assume it's a file.
    return (file_attr != INVALID_FILE_ATTRIBUTES &&
            !(file_attr & FILE_ATTRIBUTE_DIRECTORY));
}

static Str8
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

Str8 get_file_abspath(MemoryArena *allocator, Str8 file_path)
{
    static char buffer[256];
    u32 len = GetFullPathNameA(file_path.str, 256, buffer, NULL);
	
    return get_file_unixpath(allocator, Str8(allocator, str8_from_cstr(buffer)));
}

Str8List get_dir_list_ext(MemoryArena *allocator, 
                          Str8 dir_path, Str8 ext)
{
    Str8List dir_file_list{};
	
    WIN32_FIND_DATAA find_data{};
    HANDLE search_handle{};
	
    // TODO(sir->w7): Is there any way to get a list of all files in the directory without hacking together a path?
    {
        // NOTE(sir->w7): Win32 API is pretty dumb, so we need to rework the string while keeping compatibility with Linux.
        TempArena scratch(allocator);
        
        Str8 dir_path_hacked = get_file_unixpath(allocator, dir_path);
        if (dir_path.str[dir_path.len - 1] == '/') {
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
            Str8 file_rel = push_str8_concat(allocator, dir_path, str8_lit("/"));
            file_rel = push_str8_concat(allocator, file_rel, filename);
            
            dir_file_list.push(allocator, get_file_abspath(allocator, file_rel));
        }
    } while (FindNextFileA(search_handle, &find_data));
	
    FindClose(search_handle);
	
    return dir_file_list;
}
