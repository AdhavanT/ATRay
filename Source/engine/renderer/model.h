#pragma once
#include "material.h"
#include "ray.h"

extern f32 tolerance;

struct TriangleVertices
{
	vec3f a;
	vec3f b;
	vec3f c;
};

struct Face
{
	int32 vertex_indices[3] = { 0 };
	int32 tex_coord_indices[3] = { 0 };
	int32 vertex_normals_indices[3] = { 0 };
};

struct ModelData
{
	FDBuffer<vec3f, uint32> vertices;
	FDBuffer<vec3f, uint32> normals;
	FDBuffer<vec3f, uint32> tex_coords;
	FDBuffer<Face, uint32> faces;
};

struct Model
{	
	ModelData data;			
};


//Uses Moller-Trumbore intersection algorithm
//intersection_point = (1-u-v)*tri.a + u*tri.b + v*tri.c
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
inline vec3f get_scale_per_axis(Model& mdl)
{
	f32 x_max = -MAX_FLOAT, x_min = MAX_FLOAT, y_max = -MAX_FLOAT, y_min = MAX_FLOAT, z_max = -MAX_FLOAT, z_min = MAX_FLOAT;
	for (uint32 i = 0; i < mdl.data.vertices.size; i++)
	{
		x_max = max(x_max, mdl.data.vertices[i].x);
		x_min = min(x_min, mdl.data.vertices[i].x);

		y_max = max(y_max, mdl.data.vertices[i].y);
		y_min = min(y_min, mdl.data.vertices[i].y);

		z_max = max(z_max, mdl.data.vertices[i].z);
		z_min = min(z_min, mdl.data.vertices[i].z);
	}
	return (vec3f((x_max - x_min), (y_max - y_min), (z_max - z_min)));
}

void resize_scale(Model& mdl,vec3f& scale_per_axis ,f32 new_max_scale)
{
	f32 rescale_factor;
	f32 max_scale = max(max(scale_per_axis.x, scale_per_axis.y), scale_per_axis.z);
	rescale_factor = (new_max_scale / max_scale);
	for (uint32 i = 0; i < mdl.data.vertices.size; i++)
	{
		mdl.data.vertices[i] *= rescale_factor ;
	}
}