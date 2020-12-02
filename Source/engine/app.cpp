#include "app.h"
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

ATP_REGISTER(render_app);
ATP_REGISTER(load_assets);
ATP_REGISTER(prep_scene);
ATP_REGISTER(render_from_camera);
void print_out_tests();

void render_app(Texture& texture, ThreadPool& tpool)
{
	ATP_START(render_app);

	ATP_START(load_assets);

	printf("\nLoading Assets...\n");
	
	Model monkey;
	load_model_data(monkey.data, "Assets\\Monkey.obj", tpool);
	AABB monkey_scale = get_AABB(monkey);
	resize_scale(monkey, monkey_scale, 2);
	translate_to(monkey, monkey_scale, { 1.f,2.f,-3.f });

	Model monkey_aabb = make_model_from_aabb(monkey_scale);
	ATP_END(load_assets);

	Camera cm;
	RenderSettings rs;
	rs.no_of_threads = tpool.threads.size;
	rs.anti_aliasing = false;
	rs.resolution.x = texture.bmb.width;
	rs.resolution.y = texture.bmb.height;
	rs.samples_per_pixel = 1;
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
	monkey_aabb.data.material = &scene.materials[6];

	AABB tmp = {};
	tmp.min = { -0.5f,-0.5f,-3.5f };
	tmp.max = { 0.5f,4.5f,-2.5f };

	scene.models.add_nocpy(monkey_aabb);
	scene.models.add_nocpy(monkey);
	scene.planes.add(pln[0]);
	scene.planes.add(pln[1]);
	scene.spheres.add(spr[0]);
	scene.spheres.add(spr[1]);

	ATP_START(prep_scene);
	prep_scene(scene);
	ATP_END(prep_scene);

	printf("\nResolution [%i,%i] || Samples per pixel - %i - Starting Render...\n",texture.bmb.width, texture.bmb.height, rs.samples_per_pixel);
	
	ATP_START(render_from_camera);
	int64 rays_shot = render_from_camera(cm, scene, texture, tpool);
	ATP_END(render_from_camera);

	ATP_END(render_app);

	printf("\nCompleted:\n");
	
	print_out_tests();
	
	printf("	Total Rays Shot: %I64i rays\n", rays_shot);
	printf("	Millisecond Per Ray: %.*f ms/ray\n", 8, ATP::get_ms_from_test(*ATP::lookup_testtype("render_from_camera")) / (f64)rays_shot);

}

void print_out_tests()
{
	int32 length;
	ATP::TestType* front = ATP::get_testtype_registry(length);
	for (int i = 0; i < length; i++)
	{
		f64 ms = ATP::get_ms_from_test(*front);
		printf("	Time Elapsed(ATP->%s):%.*f ms (%.*f s)\n",front->name,3, ms, 4,ms/1000);
		front++;
	}
}