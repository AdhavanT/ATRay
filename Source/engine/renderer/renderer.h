#pragma once
#include "PL/PL_math.h"
#include "engine/tools/texture.h"
#include "engine/tools/work_queue.h"
#include "scene.h"
#include "camera.h"

#define USE_KD_TREE


struct RenderTile
{
	Tile tile;
	int64 ray_casts;
};

struct RenderInfo
{
	WorkQueue<RenderTile> twq;

	//buffers used for KD_tree traversal
	uint32 hit_stack_capacity;
	uint32 leaf_stack_capacity;

	Camera* camera;
	Scene* scene;
	Texture* camera_tex;

	int64 total_ray_casts = 0;
};

void start_render_from_camera(RenderInfo& info, ThreadPool& tpool);
b32 wait_for_render_from_camera_to_finish(RenderInfo& info, ThreadPool& tpool, uint32 ms_to_wait_for);

void prep_scene(Scene&, uint32& max_no_nodes_from_all_kd_trees);