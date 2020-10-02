#pragma once
#include "utilities/types.h"


struct Ray
{
	vec3f origin, direction;
	inline vec3f at( f32 distance)
	{
		return origin + (direction * distance);
	}
};

static inline void SetRay(Ray &ray,vec3f origin_, vec3f towards_)
{
	ray.origin = origin_;
	ray.direction = towards_ - origin_;
	normalize(ray.direction);
}
