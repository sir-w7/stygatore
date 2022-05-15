#include <time.h>
#include <sys/mman.h>

#include "../layer.h"
#include "../platform.h"

f32 get_time()
{
	struct timespec time_spec_thing = {0};
	clock_gettime(CLOCK_REALTIME, &time_spec_thing);
	float seconds_elapsed =
    ((float)time_spec_thing.tv_nsec / (float)1e9);
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
