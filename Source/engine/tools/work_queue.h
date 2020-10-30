#pragma once

#include <utilities/types.h>
#include "thread_pool.h"

template<typename t, typename size_type = int32>
struct WorkQueue
{
	FDBuffer<t> jobs;
	volatile size_type jobs_done = 0;
};

