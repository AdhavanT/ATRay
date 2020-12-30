#pragma once
#include "material.h"
#include "aabb.h"
#include "ray.h"

struct TriangleVertices
{
	vec3f a;
	vec3f b;
	vec3f c;
};

struct FaceVertices
{
	int32 vertex_indices[3] = { 0 };
};

struct TriangleIntersectionData
{
	uint32 face_index;
	f32 u, v;
};

struct FaceData
{
	int32 tex_coord_indices[3] = { 0 };
	int32 vertex_normals_indices[3] = { 0 };
};

struct ModelData
{
	Material* material;
	FDBuffer<vec3f, uint32> vertices;				//list of vertices in model
	FDBuffer<vec3f, uint32> normals;				//list of normals in model
	FDBuffer<vec3f, uint32> tex_coords;				//list of texture coordinates in model
	FDBuffer<FaceVertices, uint32> faces_vertices;	//list of face vertices 
	FDBuffer<FaceData, uint32> faces_data;			//data for respective faces (same index for faces_vertices)
};

//Gets the scale per axis of the model
inline AABB get_AABB(ModelData& data)
{
	f32 x_max = -MAX_FLOAT, x_min = MAX_FLOAT, y_max = -MAX_FLOAT, y_min = MAX_FLOAT, z_max = -MAX_FLOAT, z_min = MAX_FLOAT;
	for (uint32 i = 0; i < data.vertices.size; i++)
	{
		x_max = max(x_max, data.vertices[i].x);
		x_min = min(x_min, data.vertices[i].x);

		y_max = max(y_max, data.vertices[i].y);
		y_min = min(y_min, data.vertices[i].y);

		z_max = max(z_max, data.vertices[i].z);
		z_min = min(z_min, data.vertices[i].z);
	}
	AABB ret = {};
	ret.max = { x_max,y_max,z_max };
	ret.min = { x_min, y_min, z_min };
	ret.max += tolerance;
	ret.min -= tolerance;
	return (ret);
}

//NOTE: Placed here cause file uses definitions defined above
#include "kd_tree.h"

struct Model
{	
	KD_Tree kd_tree;
	ModelData data;			
	AABB surrounding_aabb;
};

//Uses Moller-Trumbore intersection algorithm
//intersection_point = (1-u-v)*tri.a + u*tri.b + v*tri.c
static FORCEDINLINE f32 get_triangle_ray_intersection_culled(Ray& ray, TriangleVertices& tri, f32& u, f32& v)
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



//Resizes the model into a max scale 
inline void resize_scale(Model& mdl ,f32 new_max_scale)
{
	AABB& bounding_box = mdl.surrounding_aabb;
	f32 rescale_factor;
	vec3f scale_range = bounding_box.max - bounding_box.min;
	f32 max_scale = max(max(scale_range.x, scale_range.y), scale_range.z);
	rescale_factor = (new_max_scale / max_scale);
	for (uint32 i = 0; i < mdl.data.vertices.size; i++)
	{
		mdl.data.vertices[i] *= rescale_factor ;
	}
	bounding_box.max = bounding_box.max * rescale_factor;
	bounding_box.min = bounding_box.min * rescale_factor;
}

inline void translate_to(Model& mdl, vec3f new_center)
{
	AABB& aabb = mdl.surrounding_aabb;

	vec3f old_center = aabb.min + (aabb.max - aabb.min) / 2;

	//vec3f old_center = (aabb.max - aabb.min) / 2;
	vec3f translation = new_center - old_center;

	for (uint32 i = 0; i < mdl.data.vertices.size; i++)
	{
		mdl.data.vertices[i] += translation;
	}

	aabb.max += translation;
	aabb.min += translation;
}
