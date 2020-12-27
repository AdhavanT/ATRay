#pragma once
#include "PL/PL_utils.h"

struct Thread
{
	ThreadHandle handle;
};

struct ThreadPool
{
	FDBuffer<Thread> threads;
};

inline void activate_pool(ThreadPool& pool, ThreadProc proc, void* data)
{
	for (int i = 0; i < pool.threads.size; i++)
	{
		pool.threads[i].handle = pl_create_thread(proc, data);
	}
}

//waits for all threads to be released. Returns TRUE if wait is timed out, and FALSE if all threads are finished
inline b32 wait_for_pool(ThreadPool& pool, uint32 time_out_in_ms)
{
	return pl_wait_for_all_threads(pool.threads.size,&pool.threads[0].handle, time_out_in_ms);
}
