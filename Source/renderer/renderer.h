#pragma once
#include "utilities/types.h"
#include "utilities/texture.h"
#include "utilities/atp.h"
#include "platform/platform.h"
#include "tile_work_queue.h"
#include "scene.h"
#include <renderer\camera.h>

enum class IntersectionType
{
	NO_INTERSECTION = 0, CLOSEST
};

struct Intersection
{
	IntersectionType type;
	vec3f closest = { 0,0,0 };
};



struct RenderInfo
{
	TileWorkQueue twq;

	Camera* camera;
	Scene* scene;
	Texture* camera_tex;

	volatile int64 total_ray_casts = 0;
};




int64 render_from_camera(Camera& cm, Scene& scene, Texture& tex);

void prep_scene(Scene);
