#include "app.h"
#include "utilities/atp.h"
#include "engine/OBJ_loader.h"


ATP_REGISTER(render_app);
ATP_REGISTER(load_assets);
ATP_REGISTER(prep_scene);
ATP_REGISTER(render_from_camera);

//ASSESS: How I wanna store a "Face" and whether I want to parse the model like this and do pre-processing later


void print_out_tests();

void render_app(Texture& texture, ThreadPool& tpool)
{
	ATP_START(render_app);

	ATP_START(load_assets);

	printf("\nLoading Assets...\n");
	Model deer;
	load_model(deer.data, "Assets\\Deer.obj", tpool);
	AABB deer_scale = get_AABB(deer);
	resize_scale(deer, deer_scale, 2);
	translate_to(deer, deer_scale, { 3.f,2.f,-7.f });
	ATP_END(load_assets);

	Camera cm;
	RenderSettings rs;
	rs.no_of_threads = tpool.threads.size;
	rs.anti_aliasing = true;
	rs.resolution.x = texture.bmb.width;
	rs.resolution.y = texture.bmb.height;
	rs.samples_per_pixel = 16;
	rs.bounce_limit = 5;

	set_camera(cm, { 0.f,1.f,0.f }, { 0.f, 0.0f,-1.f }, rs, 1.0f);

	Scene scene;
	Material skybox;
	Material sphere_1 = { {0.f,0.8f,0.9f }, 0.3f };
	Material sphere_2 = { {0.9f,0.4f,0.2f }, 0.9f };
	Material plane_2 = { { 0.2f, 0.3f,0.2f } , 0.f };
	Material ground_plane = { {0.9f, 0.8f,0.2f } , 0.f };

	scene.materials.add(skybox);	//The first material is the skybox
	scene.materials.add(sphere_1);
	scene.materials.add(sphere_2);
	scene.materials.add(plane_2);
	scene.materials.add(ground_plane);

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


	pln[0].distance = 7.f;
	pln[0].normal = { -1.f,0.f,0.f };
	pln[0].material = &scene.materials[3];


	//ground plane
	pln[1].distance = 0.f;
	pln[1].normal = { 0.f,1.f,0.f };
	pln[1].material = &scene.materials[4];

	scene.models.add_nocpy(deer);
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
		printf("	Time Elapsed(ATP->%s):%.*f ms\n",front->name,3, ATP::get_ms_from_test(*front));
		front++;
	}
}