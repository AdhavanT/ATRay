#pragma once
#include "material.h"
#include "ray.h"

struct Sphere
{
	vec3f center;
	f32 radius;
	Material* material;
};

static inline f32 get_sphere_ray_intersection(Ray& ray, Sphere& sphere)
{
	vec3f p_to_c = (ray.origin - sphere.center);
	f32 p_to_c_sqr = mag2(p_to_c);
	f32 b = 2 * (dot(ray.direction, p_to_c));
	f32 b_sqr = b*b;
	f32 c = p_to_c_sqr - sphere.radius* sphere.radius;

	f32 dmt = b_sqr - (4 * c);

	if (dmt < 0)
	{
		return 0;
	}
	f32 ta, tb;
	ta = (-b + sqroot(dmt)) * 0.5f;
	tb = (-b - sqroot(dmt)) * 0.5f;
	if (ta <= 0 && tb <= 0)	//both intersections are behind the ray 
	{
		return 0;
	}
	if (tb > 0)				//tb is the shortest distance, so checking if its positive and return it
	{
		return tb;
	}
	return ta;

}
