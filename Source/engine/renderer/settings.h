#pragma once
#include "PL/PL_math.h"

struct RenderSettings
{
	vec2i resolution;
	b32 anti_aliasing;
	uint32 samples_per_pixel;
	int32 bounce_limit;
};