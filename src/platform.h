#ifndef PLATFORM_H
#define PLATFORM_H

f64 get_time();

void *reserve_mem(u64 size);
void free_mem(void *mem, u64 size);

b32 is_file(Str8 path);
b32 is_dir(Str8 path);

Str8 get_file_abspath(MemoryArena *allocator, Str8 file_path);
Str8List get_dir_list_ext(MemoryArena *allocator, Str8 dir, Str8 ext);

struct ThreadContext
{
	u32 thr_idx;

	MemoryArena allocator;
	MemoryArena temp_allocator;
};

typedef void HandleFunc(MemoryArena *, MemoryArena *, Str8);

void init_pools(MemoryArena *allocator, u32 cnt);
void queue_job(HandleFunc *func, Str8 data);
void wait_pools();
void send_kill_signals();

template <typename T>
struct Profiler {
    Profiler(T lambda, Str8 identifier) {
    	auto time_start = get_time();
    	lambda();
    	println("    -- INFO (styx_profiler): %.*s - %f ms.", str8_exp(identifier), get_time() - time_start);
    }
};

struct ProfilerHelper {
	Str8 identifier;

	ProfilerHelper(Str8 identifier) : identifier(identifier) {}

    template <typename T>
    Profiler<T> operator+(T t){ return Profiler<T>(t, identifier); }
};

#if STYX_DEBUG
	#define profile_block(block_name) ProfilerHelper(str8_lit(block_name)) + [&]()
	#define profile_def(name) \
		auto __time_start_##__LINE__ = get_time(); \
		defer { println("    -- INFO (styx_profiler): " name " - %f ms.", get_time() - __time_start_##__LINE__); }
#else
	#define profile_block(block_name)
	#define profile_def(name)
#endif

#endif
