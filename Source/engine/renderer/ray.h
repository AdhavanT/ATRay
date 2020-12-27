#pragma once
#include "PL/PL_math.h"

static constexpr f32 tolerance = 0.0001f;


struct Ray
{
	vec3f origin, direction;
	inline vec3f at( f32 distance)
	{
		return origin + (direction * distance);
	}
};

struct Optimized_Ray
{
	Ray ray;
	vec3f inv_ray_d;
	vec3i inv_signs;
};

static inline void SetRay(Ray &ray,vec3f origin_, vec3f towards_)
{
	ray.origin = origin_;
	ray.direction = towards_ - origin_;
	normalize(ray.direction);
}
