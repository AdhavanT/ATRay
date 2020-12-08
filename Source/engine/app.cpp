#include "PL/pl.h"
#include "renderer/renderer.h"
#include "utilities/texture.h"
#include "utilities/ATP/atp.h"
#include "engine/tools/OBJ_loader.h"

//TODO: remove this. This is for debug purposes. 
static Model make_model_from_aabb(AABB box)
{
	Model mdl = {};
	int32 no_faces = 6 * 2;
	int32 no_vertices = 8;
	mdl.data.faces_vertices.allocate(no_faces);
	mdl.data.vertices.allocate(no_vertices);
	mdl.data.vertices[0] = box.min;
	mdl.data.vertices[1] = { box.max.x, box.min.y,box.min.z };
	mdl.data.vertices[2] = { box.min.x, box.max.y,box.min.z };
	mdl.data.vertices[3] = { box.max.x, box.max.y,box.min.z };
	mdl.data.vertices[4] = { box.min.x, box.min.y,box.max.z };
	mdl.data.vertices[5] = { box.max.x, box.min.y,box.max.z };
	mdl.data.vertices[6] = { box.min.x, box.max.y,box.max.z };
	mdl.data.vertices[7] = box.max;

	mdl.data.faces_vertices[0] = { 0,1,3 };
	mdl.data.faces_vertices[1] = { 0,3,2 };
	mdl.data.faces_vertices[2] = { 1,5,4 };
	mdl.data.faces_vertices[3] = { 1,4,0 };
	mdl.data.faces_vertices[4] = { 2,6,4 };
	mdl.data.faces_vertices[5] = { 2,4,0 };
	mdl.data.faces_vertices[6] = { 3,6,2 };
	mdl.data.faces_vertices[7] = { 3,7,6 };
	mdl.data.faces_vertices[8] = { 4,5,7 };
	mdl.data.faces_vertices[9] = { 4,7,6 };
	mdl.data.faces_vertices[10] = { 5,7,3 };
	mdl.data.faces_vertices[11] = { 5,3,1 };
	return mdl;
}

ATP_REGISTER(load_assets);
ATP_REGISTER(prep_scene);
ATP_REGISTER(render_from_camera);
void print_out_tests(PL_Timing& pl);
void render_app(PL& pl, Texture& texture, ThreadPool& tpool);

void PL_entry_point(PL& pl)
{
	pl.running = TRUE;
	pl.core_count = 8;
	Texture texture;
	Setup_Texture(texture, TextureFileType::BMP, 1270, 720);

	pl.window.height = texture.bmb.height;
	pl.window.width = texture.bmb.width;
	pl.window.title = (char*)"Starting Rendering...";
	pl.window.window_bitmap.buffer = texture.bmb.buffer_memory;

	ThreadPool thread_pool;
	//thread_pool.threads.allocate(1);	//For single thread
	thread_pool.threads.allocate(pl.core_count);

	PL_initialize_input_keyboard(pl.input.kb);
	PL_initialize_input_mouse(pl.input.mouse);
	PL_initialize_window(pl.window);
	PL_initialize_timing(pl.time);
	render_app(pl,texture, thread_pool);

	buffer_free(texture.bmb.buffer_memory);
	
	//NOTE: by now, none of the threads should be running anything. 
	close_threads((uint32)thread_pool.threads.size, &thread_pool.threads.front->handle);
	thread_pool.threads.clear();
}

static int32 find_tile_index_covering_point(vec2i point, WorkQueue<RenderTile>& twq)
{
	for (uint32 i = 0; i < (uint32)twq.jobs.size; i++)
	{
		if (twq.jobs[i].tile.left_bottom.x <= point.x && twq.jobs[i].tile.left_bottom.y <= point.y &&
			twq.jobs[i].tile.right_top.x >= point.x && twq.jobs[i].tile.right_top.y >= point.y)
		{
			return (int32)i;
		}
	}
	return -1;
}

static void render_app(PL& pl,Texture& texture, ThreadPool& tpool)
{

	ATP_START(load_assets);

	debug_print("\nLoading Assets...\n");
	
	Model monkey = {};
	load_model_data(monkey.data, "Assets\\Monkey.obj", tpool);
	monkey.surrounding_aabb = get_AABB(monkey);
	resize_scale(monkey, 2);
	translate_to(monkey, { 1.f,2.f,-3.f });

	//Model monkey_aabb = make_model_from_aabb(monkey_scale);
	ATP_END(load_assets);

	Camera cm;
	RenderSettings rs;
	rs.no_of_threads = tpool.threads.size;
	rs.anti_aliasing = TRUE;
	rs.resolution.x = texture.bmb.width;
	rs.resolution.y = texture.bmb.height;
	rs.samples_per_pixel = 5;
	rs.bounce_limit = 5;


	set_camera(cm, { 0.f,4.f,0.f }, { 0.f, -1.f,-1.f }, rs, 1.0f);

	Scene scene;
	Material skybox = { {0.3f,0.4f,0.5f}, {0.2f,0.3f,0.4f},0.3f };
	Material sphere_1 = { {0.f,0.0f,0.0f }, {0.2f,0.8f,0.2f },0.3f };
	Material sphere_2 = { {0.0f,0.0f,0.0f }, {0.4f,0.8f,0.9f },0.9f };
	Material plane_2 = { { 0.0f, 0.4f,0.6f } , { 0.2f, 0.3f,0.2f },0.f };
	Material ground_plane = { {0.f, 0.f,0.0f } , {0.5f, 0.5f,0.5f },0.f };
	Material model = { {0.4f,0.2f,0.2f}, {0.92f,0.5f,0.0f},0.3f };
	Material mat_model_aabb = { {0.8f,0.2f,0.2f}, {0.92f,0.0f,0.0f},0.3f };

	scene.materials.add(skybox);	//The first material is the skybox
	scene.materials.add(sphere_1);
	scene.materials.add(sphere_2);
	scene.materials.add(plane_2);
	scene.materials.add(ground_plane);
	scene.materials.add(model);
	scene.materials.add(mat_model_aabb);

	//NOTE: this stuff is ad-hoc right now and should be properly implemented. 
	//Should Objects contain a pointer to a material or should they have an "index" instead...
	//Depends on how materials are loaded and what order they are stored in. Objects need to know which material
	// they point to in the Material buffer in scene.
	Sphere spr[2];
	Plane pln[2];

	spr[0].center = { -1.f,1.0f,-7.f };
	spr[0].radius = 1.0f;
	spr[0].material = &scene.materials[1];

	spr[1].center = { 1.f,1.f,-7.f };	
	spr[1].radius = 1.f;    
	spr[1].material = &scene.materials[2];


	pln[0].distance = -7.f;
	pln[0].normal = { 1.f,0.f,0.f };
	pln[0].material = &scene.materials[3];

	//ground plane
	pln[1].distance = 0.f;
	pln[1].normal = { 0.f,1.f,0.f };
	pln[1].material = &scene.materials[4];

	monkey.data.material = &scene.materials[5];
	//monkey_aabb.data.material = &scene.materials[6];

	AABB tmp = {};
	tmp.min = { -0.5f,-0.5f,-3.5f };
	tmp.max = { 0.5f,4.5f,-2.5f };

	//scene.models.add_nocpy(monkey_aabb);
	scene.models.add_nocpy(monkey);
	scene.planes.add(pln[0]);
	scene.planes.add(pln[1]);
	scene.spheres.add(spr[0]);
	scene.spheres.add(spr[1]);

	ATP_START(prep_scene);
	prep_scene(scene);
	ATP_END(prep_scene);

	debug_print("\nResolution [%i,%i] || Samples per pixel - %i - Starting Render...\n",texture.bmb.width, texture.bmb.height, rs.samples_per_pixel);
	
	RenderInfo info;
	info.camera_tex = &texture;
	info.camera = &cm;
	info.scene = &scene;

	ATP_START(render_from_camera);
	start_render_from_camera(info, tpool);

	int32 last_tile = 0;
	while (wait_for_render_from_camera_to_finish(info, tpool, 33))	//checks if threadpool is finished every 33 milliseconds and exists when done.
	{
		PL_poll_window(pl.window);
		if (pl.running != TRUE)
		{
			//TODO:Maybe put warning message about how the process wont end until all threads finish the tile they're working on
			info.twq.clear();
			wait_for_pool(tpool, UINT32MAX);
			free_scene_memory(scene);	//NOTE:cleared out on process end anyway. Maybe implement a "shutdown" which waits for all threads to finish then clears scene memory.
			return;

		}
		if (info.twq.jobs_done > last_tile)
		{
			char buffer[512];
			format_print(buffer, 512, "Rendering: Width:%i, Height:%i | Threads: %i | Tiles: %i/%i", pl.window.width, pl.window.height, pl.core_count, info.twq.jobs_done, info.twq.jobs.size);
			pl.window.title = buffer;
			PL_push_window(pl.window, TRUE);
			last_tile = info.twq.jobs_done;
		}
		else
		{
			PL_push_window(pl.window, FALSE);
		}
	}
	ATP_END(render_from_camera);
	

	debug_print("\nCompleted:\n");
	
	print_out_tests(pl.time);
	
	debug_print("	Total Rays Shot: %I64i rays\n", info.total_ray_casts);
	debug_print("	Millisecond Per Ray: %.*f ms/ray\n", 8, ATP::get_ms_from_test(*ATP::lookup_testtype("render_from_camera")) / (f64)info.total_ray_casts);

	int32 tile_on_mouse = -1;
	ATP::TestType* Tiles_TestType = 0;
	Tiles_TestType = ATP::lookup_testtype("Tiles");
	while (pl.running)
	{

		PL_poll_window(pl.window);
		PL_poll_input_keyboard(pl.input.kb);
		PL_poll_input_mouse(pl.input.mouse, pl.window);
		
		if (pl.input.keys[PL_KEY::SHIFT].down && pl.input.keys[PL_KEY::S].down)
		{
			Write_To_File(texture, "Results\\resultings");
			pl.window.title = (char*)"SAVED TO FILE!";
			PL_push_window(pl.window, TRUE);
		}
		if (pl.input.keys[PL_KEY::ESCAPE].pressed)
		{
			pl.running = FALSE;
		}
		
		if (pl.input.mouse.left.down && pl.input.mouse.is_in_window)
		{
			vec2i point = { pl.input.mouse.position_x, pl.input.mouse.position_y };
			
			tile_on_mouse = find_tile_index_covering_point(point, info.twq);
			if (tile_on_mouse != -1)
			{
				uint64 cycles_to_render_tile = Tiles_TestType->tests.front[tile_on_mouse].test_run_cycles;
				uint64 ray_casts_on_tile = info.twq.jobs[tile_on_mouse].ray_casts;
				f64 tile_ms = ((f64)cycles_to_render_tile / (f64)pl.time.cycles_per_second) * 1000;
				char buffer[512];
				format_print(buffer, 512, "Rendered:Tile(%i/%i) milliseconds to render tile:%.*f ms | rays cast on tile:%i64  ",tile_on_mouse+1,info.twq.jobs.size, 3,tile_ms, ray_casts_on_tile);
				pl.window.title = buffer;
				PL_push_window(pl.window, TRUE);
			}
			
		}
		else
		{
			PL_push_window(pl.window, FALSE);
		}
		
	}


	if (info.twq.jobs_done == info.twq.jobs.size)
	{
		info.twq.jobs.clear();
	}
	else
	{
		ASSERT(false);	//All tiles not rendered!
	}


	free_scene_memory(scene);

}

static void print_out_tests(PL_Timing& pl)
{
	int32 length = ATP::testtype_registry->no_of_testtypes;
	ATP::TestType* front = ATP::testtype_registry->front;
	for (int i = 0; i < length; i++)
	{
		if (front->type == ATP::TestTypeFormat::MULTI)
		{
			debug_print("	MULTI TEST (ATP->%s):\n", front->name);
			ATP::TestInfo* index = front->tests.front;
			uint64 total = 0;
			for (uint32 i = 0; i < front->tests.size; i++)
			{
				total += index->test_run_cycles;
				f64 ms = (index->test_run_cycles * 1000 / (f64)pl.cycles_per_second);
				debug_print("		index:%i:%.*f ms (%.*f s),%I64u\n", i, 3, ms, 4, ms / 1000, index->test_run_cycles);
				index++;
			}
			f64 ms = (total * 1000 / (f64)pl.cycles_per_second);
			debug_print("	total:%.*f ms (%.*f s), %I64u\n", 3, ms, 4, ms / 1000, total);

		}
		else
		{
			f64 ms = ATP::get_ms_from_test(*front);
			debug_print("	Time Elapsed(ATP->%s):%.*f ms (%.*f s)\n", front->name, 3, ms, 4, ms / 1000);
		}
		front++;
	}
}