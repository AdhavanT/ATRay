#pragma once
#include "utilities/types.h"

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
		pool.threads[i].handle = create_thread(proc, data);
	}
}

inline void wait_for_pool(ThreadPool& pool)
{
	wait_for_all_threads(pool.threads.size,&pool.threads[0].handle, UINT32MAX);
	close_threads(pool.threads.size,&pool.threads[0].handle);	
}