#pragma once
#include "custom headers/types.h"
#include "custom headers/bitmap.h"
#include "custom headers/atp.h"
#include "platform/platform.h"

struct Camera
{
	f32 aspect_ratio;
	vec3f camera_z, camera_x, camera_y;

	vec3f eye; //position of virtual eye
	vec3f facing_towards; //direction of camera 
	vec3f frame_center; //center of camera "sensor"

	b32 toggle_anti_aliasing; //toggles pixel-jittering per sample for anti-aliasing
	uint32 samples_per_pixel;

	f32 h_fov;
	vec2i resolution;

	f32 half_pixel_width, half_pixel_height;
};

struct LS_Point
{
	vec3f position;
	vec3f color;

};

struct Ray
{
	vec3f origin, direction;
	inline void SetRay(vec3f origin_, vec3f towards_)
	{
		origin = origin_;
		direction = towards_ - origin_;
		normalize(direction);
	}
	inline vec3f at(f32 distance)
	{
		return origin + (direction * distance);
	}

};

enum class IntersectionType
{
	NO_INTERSECTION = 0, CLOSEST
};

struct Intersection
{
	IntersectionType type;
	vec3f closest = { 0,0,0 };
};

struct Material
{
	vec3f color;
	f32 specularity;

};

struct Sphere
{
	vec3f center;
	f32 radius;
	Material material;
};

struct Plane
{
	vec3f normal;
	f32 distance;	//this is the distance of the plane from the origin along its normal. i.e: shortest distance from origin. (can be negative or positive for both possible planes)
	Material material;
};

struct Scene
{
	int32 no_of_LS_points;
	LS_Point* ls_points;
	int32 no_of_spheres;
	Sphere* spheres;
	int32 no_of_planes;
	Plane* planes;

};

struct SceneInfo
{
	Camera* camera;
	Scene* scene;
	BitmapBuffer* bmb;
	int32 ray_bounce_limit;
};


struct TileWorkQueue
{
	SceneInfo info;
	int64 no_of_tiles;
	Tile* tiles;
	volatile int64 current_tile = 0;
	volatile int64 total_ray_casts = 0;
};


int64 render_from_camera(Camera& cm, Scene& scene, BitmapBuffer& bmb, int32 ray_bounce_limit);


vec3f cast_ray(Ray& ray, Scene& scene, int32 bounce_limit, int64& ray_casts);