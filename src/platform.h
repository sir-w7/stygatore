#ifndef PLATFORM_H
#define PLATFORM_H

f32 get_time();
void *reserve_mem(u64 size);
void free_mem(void *mem, u64 size);

#endif