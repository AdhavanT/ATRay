#pragma once
#include "material.h"
#include "ray.h"

extern f32 tolerance;

struct TriangleVertices
{
	vec3f a, b, c;
};

struct Model
{	
	int32 no_of_triangles;
	TriangleVertices* triangles;
};

//Uses Moller-Trumbore intersection algorithm
static inline f32 get_triangle_ray_intersection_culled(Ray& ray, TriangleVertices& tri, f32& u, f32& v)
{
	vec3f ab, ac;
	ab = tri.b - tri.a;
	ac = tri.c - tri.a;

	vec3f pvec = cross(ray.direction, ac);
	f32 det = dot(ab, pvec);

	//culling ( doesn't intersect if triangle and ray are facing same way)
	//NOTE: a bit faster as doesn't have to check if fabs(det) < tolerance for no culling
	if (det < tolerance)
	{
		return 0;
	}

	f32 det_inv = 1 / det;

	vec3f tvec = ray.origin - tri.a;
	u = dot(tvec, pvec) * det_inv;
	if (u < 0 || u > 1) return 0;

	vec3f qvec = cross(tvec, ab);
	v = dot(ray.direction, qvec) * det_inv;
	if (v < 0 || u + v > 1) return 0;

	return dot(qvec, ac) * det_inv; //t

}