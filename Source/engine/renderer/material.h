#pragma once
#include "PL/PL_math.h"

struct Material
{
	vec3f emission_color;
	vec3f reflection_color;
	f32 scatter;
};