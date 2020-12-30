#pragma once

#include "ray.h"

struct AABB
{
    union 
    {
        vec3f bounds[2];
        struct
        {
            //NOTE:dont change order. ray_aabb_intersection uses bounds as optimization.
            vec3f min;
            vec3f max;
        };
    };
};

static inline b8 is_inside(vec3f point, AABB box)
{
	//TODO: Make this SIMD
	b8 x_check = (point.x >= box.min.x) && (point.x <= box.max.x);
	b8 y_check = (point.y >= box.min.y) && (point.y <= box.max.y);
	b8 z_check = (point.z >= box.min.z) && (point.z <= box.max.z);

	return(x_check && y_check && z_check);
}


static inline b32 get_ray_AABB_intersection(Optimized_Ray& r, AABB& bb)
{
	/*if (is_inside(r.ray.origin, bb))
	{
		return true;
	}*/

	//optimized version 
	float tmin, tmax, tymin, tymax, tzmin, tzmax;

	tmin = (bb.bounds[r.inv_signs[0]].x - r.ray.origin.x) * r.inv_ray_d.x;
	tmax = (bb.bounds[1 - r.inv_signs[0]].x - r.ray.origin.x) * r.inv_ray_d.x;
	tymin = (bb.bounds[r.inv_signs[1]].y - r.ray.origin.y) * r.inv_ray_d.y;
	tymax = (bb.bounds[1 - r.inv_signs[1]].y - r.ray.origin.y) * r.inv_ray_d.y;

	if ((tmin > tymax) || (tymin > tmax))
		return false;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	tzmin = (bb.bounds[r.inv_signs[2]].z - r.ray.origin.z) * r.inv_ray_d.z;
	tzmax = (bb.bounds[1 - r.inv_signs[2]].z - r.ray.origin.z) * r.inv_ray_d.z;

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;

	return true;

	//better to understand what is going on
	/*float tmin = (bb.min.x - r.ray.origin.x) * r.inv_ray_d.x;
	float tmax = (bb.max.x - r.ray.origin.x) * r.inv_ray_d.x;

	if (r.inv_ray_d.x < 0) swap(tmin, tmax);

	float tymin = (bb.min.y - r.ray.origin.y) * r.inv_ray_d.y;
	float tymax = (bb.max.y - r.ray.origin.y) * r.inv_ray_d.y;

	if (r.inv_ray_d.y < 0) swap(tymin, tymax);

	if ((tmin > tymax) || (tymin > tmax))
		return false;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	float tzmin = (bb.min.z - r.ray.origin.z) * r.inv_ray_d.z;
	float tzmax = (bb.max.z - r.ray.origin.z) * r.inv_ray_d.z;

	if (r.inv_ray_d.z < 0) swap(tzmin, tzmax);

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;

	if (tzmin > tmin)
		tmin = tzmin;

	if (tzmax < tmax)
		tmax = tzmax;

	return true;
	*/
}