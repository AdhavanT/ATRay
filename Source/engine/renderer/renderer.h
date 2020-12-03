#pragma once
#include "utilities/types.h"
#include "utilities/texture.h"
#include "engine/tools/work_queue.h"
#include "scene.h"
#include "camera.h"

struct RenderTile
{
	Tile tile;
	int64 ray_casts;
	uint64 cycles_to_render;
};

struct RenderInfo
{
	WorkQueue<RenderTile> twq;

	Camera* camera;
	Scene* scene;
	Texture* camera_tex;

	int64 total_ray_casts = 0;
};

void start_render_from_camera(RenderInfo& info, ThreadPool& tpool);
b32 wait_for_render_from_camera_to_finish(RenderInfo& info, ThreadPool& tpool, uint32 ms_to_wait_for);


void prep_scene(Scene);
