#pragma once
#include "PL/PL_math.h"

struct RenderSettings
{
	uint32 no_of_threads;
	vec2i resolution;
	b32 anti_aliasing;
	uint32 samples_per_pixel;
	int32 bounce_limit;
};