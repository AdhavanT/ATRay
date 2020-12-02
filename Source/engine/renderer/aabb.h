#pragma once

#include "ray.h"

struct AABB
{
    union 
    {
        vec3f bounds[2];
        struct
        {
            vec3f max;
            vec3f min;
        };
    };
};


static inline b32 is_inside(vec3f& point, AABB& box)
{
	//TODO: Make this SIMD
	b32 x_check = (point.x >= box.min.x) & (point.x <= box.max.x);
	b32 y_check = (point.y >= box.min.y) & (point.y <= box.max.y);
	b32 z_check = (point.z >= box.min.z) & (point.z <= box.max.z);

	return(x_check & y_check & z_check);
}


//
static inline b32 get_ray_AABB_intersection(Ray& r, AABB& bb, vec3f inv_ray_d, vec3i inv_signs)
{
	if (is_inside(r.origin, bb))
	{
		return true;
	}

    float tmin, tmax, tymin, tymax, tzmin, tzmax;

    tmin = (bb.bounds[inv_signs[0]].x - r.origin.x) * inv_ray_d.x;
    tmax = (bb.bounds[1 - inv_signs[0]].x - r.origin.x) * inv_ray_d.x;
    tymin = (bb.bounds[inv_signs[1]].y - r.origin.y) * inv_ray_d.y;
    tymax = (bb.bounds[1 - inv_signs[1]].y - r.origin.y) * inv_ray_d.y;

    if ((tmin > tymax) || (tymin > tmax))
        return false;
    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    tzmin = (bb.bounds[inv_signs[2]].z - r.origin.z) * inv_ray_d.z;
    tzmax = (bb.bounds[1 - inv_signs[2]].z - r.origin.z) * inv_ray_d.z;

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    return true;

}