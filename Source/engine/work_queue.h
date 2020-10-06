#pragma once

#include <utilities/types.h>
#include "engine/thread_pool.h"

template<typename t, typename size_type = int32>
struct WorkQueue
{
	FDBuffer<t> jobs;
	volatile size_type jobs_done = 0;
};

