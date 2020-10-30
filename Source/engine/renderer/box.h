#pragma once

#include "ray.h"

struct AABB
{
	vec3f max;
	vec3f min;
};


static inline b32 is_inside(vec3f& point, AABB& box)
{
	//TODO: Make this SIMD
	b32 x_check = (point.x >= box.min.x) & (point.x <= box.max.x);
	b32 y_check = (point.y >= box.min.y) & (point.y <= box.max.y);
	b32 z_check = (point.z >= box.min.z) & (point.z <= box.max.z);

	return(x_check & y_check & z_check);
}


//TODO: Finish this!
static inline b32 check_ray_AABB_intersection(Ray& ray, AABB& bb)
{
	if (is_inside(ray.origin, bb))
	{
		return true;
	}
	//performing intersection:

	f32 t_lower_xz_plane = (bb.min.y - ray.origin.y) / ray.direction.y;
	vec3f intersection_low_xz_plane = ray.at(t_lower_xz_plane);
	b32 intersect_low_xz_plane = (intersection_low_xz_plane.x > bb.min.x) && (intersection_low_xz_plane.x < bb.max.x);
	intersect_low_xz_plane = intersect_low_xz_plane & ((intersection_low_xz_plane.z > bb.min.z) & (intersection_low_xz_plane.z < bb.max.z));

	if (intersect_low_xz_plane)
	{
		return true;
	}

}