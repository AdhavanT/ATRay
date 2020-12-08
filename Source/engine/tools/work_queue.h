#pragma once

#include "PL/PL_math.h"
#include "thread_pool.h"

template<typename t, typename size_type = int32>
struct WorkQueue
{
	FDBuffer<t, volatile int32> jobs;
	volatile size_type jobs_done = 0;
	void clear()
	{
		interlocked_exchange_i32(&jobs.size, 0);
		jobs.clear();
	}
};



