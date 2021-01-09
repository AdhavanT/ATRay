#pragma once
#include "material.h"
#include "ray.h"

struct Plane
{
	vec3f normal = { 0 };
	f32 distance = 0;	//this is the distance of the plane from the origin along its normal. i.e: shortest distance from origin. (can be negative or positive for both possible planes)
	Material* material = 0;
};

static inline f32 get_plane_ray_intersection(Ray& ray, Plane& plane)
{
	//assumes plane normal is normalized
	f32 denom = dot(plane.normal, ray.direction);
	if (denom > -tolerance && denom < tolerance)
	{
		return 0;
	}
	f32 t = (plane.distance - dot(ray.origin, plane.normal)) / denom;
	return t;
}
