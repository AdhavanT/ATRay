#pragma once
#include "material.h"
#include "ray.h"

extern f32 tolerance;

struct Plane
{
	vec3f normal;
	f32 distance;	//this is the distance of the plane from the origin along its normal. i.e: shortest distance from origin. (can be negative or positive for both possible planes)
	Material material;
};

static inline f32 get_plane_ray_intersection(Ray& ray, Plane& plane)
{
	//assumes plane normal is normalized
	f32 denom = dot(plane.normal, ray.direction);
	if (denom > -tolerance && denom < tolerance)
	{
		return 0;
	}
	f32 t = (-plane.distance - dot(ray.origin, plane.normal)) / denom;
	return t;
}
