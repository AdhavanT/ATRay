#include "app.h"
#include "custom headers/atp.h"

ATP_REGISTER(render_app);
ATP_REGISTER(prep_scene);
ATP_REGISTER(render_from_camera);

void render_app(BitmapBuffer& bmb)
{
	int32 smp = 64;
	printf("\nResolution [%i,%i] || Samples per pixel - %i - Starting Render...\n",bmb.width, bmb.height, smp);
	
	ATP_START(render_app);

	Camera cm;
	cm.eye = { 0.f,3.f,0.f };
	cm.facing_towards = { 0.f, -0.3f,-1.f };
	cm.h_fov = 1.f;
	cm.resolution.x = bmb.width;
	cm.resolution.y = bmb.height;
	cm.toggle_anti_aliasing = true;
	cm.samples_per_pixel = smp;

	Sphere spr[2];
	Plane pln[1];
	Triangle tri[1];

	tri[0].a = { -1.0f, -1.0f, -5.0f };
	tri[0].b = { 1.0f, -1.0f, -5.0f };
	tri[0].c = { 0.f, 1.0f, -5.0f };
	tri[0].material.color = { 0,0.8f,0 };
	tri[0].material.specularity = 0;

	spr[0].center = { 0.f,0.f,-7.f };
	spr[0].radius = 1.0f;
	spr[0].material.color = { 0.1f,0.8f,0.2f };
	spr[0].material.specularity = 0.4f;

	spr[1].center = { 2.f,1.f,-7.f };
	spr[1].radius = 1.f;
	spr[1].material.color = { 0.9f,0.4f,0.2f };
	spr[1].material.specularity = 0.9f;


	pln[0].material.color = { 0.2f, 0.2f,0.2f };
	pln[0].material.specularity= 0.2f;
	pln[0].distance = 0.f;
	pln[0].normal = { 0.f,1.f,0.f };

	Scene scene;
	scene.no_of_triangles = ArrayCount(tri);
	scene.no_of_planes = ArrayCount(pln);
	scene.no_of_spheres = ArrayCount(spr);
	scene.triangles = &tri[0];
	scene.spheres = &spr[0];
	scene.planes = &pln[0];

	
	ATP_START(prep_scene);
	prep_scene(scene);
	ATP_END(prep_scene);

	ATP_START(render_from_camera);
	int64 rays_shot = render_from_camera(cm, scene, bmb, 5);
	ATP_END(render_from_camera);

	ATP_END(render_app);

	printf("\nCompleted:\n");
	ATP::TestType* tt = ATP::lookup_testtype("render_app");
	f64 time_elapsed = ATP::get_ms_from_test(*tt);

	ATP::TestType* ps = ATP::lookup_testtype("prep_scene");
	f64 time_elapsed_ps = ATP::get_ms_from_test(*ps);

	ATP::TestType* rc = ATP::lookup_testtype("render_from_camera");
	f64 time_elapsed_rc = ATP::get_ms_from_test(*rc);


	printf("	Time Elapsed(ATP->render_app):%.*f seconds\n", 3, time_elapsed / 1000);
	printf("	Time Elapsed(ATP->prep_scene):%.*f ms\n", 3, time_elapsed_ps);
	printf("	Time Elapsed(ATP->render_from_camera):%.*f seconds\n", 3, time_elapsed_rc / 1000);

	printf("	Total Rays Shot: %I64i rays\n", rays_shot);
	printf("	Millisecond Per Ray: %.*f ms/ray\n", 8, time_elapsed_rc / (f64)rays_shot);

}


ATP_REGISTER(PCG_Rand);
ATP_REGISTER(rand);

void benchmark()
{
	RNG_Stream st = { 0,7656776 };
	f32 thing = 0, thing2 = 0;
	ATP_START(PCG_Rand);
	for (int i = 0; i < 10000000; i++)
	{
		thing += rand_uni(&st);
	}
	ATP_END(PCG_Rand);

	ATP_START(rand);
	for (int i = 0; i < 10000000; i++)
	{
		thing2 += rand() * 0.0000305185f;
	}
	ATP_END(rand);

	printf("\nCompleted:\n");
	ATP::TestType* tt = ATP::lookup_testtype("PCG_Rand");
	f64 time_elapsed = ATP::get_ms_from_test(*tt);

	ATP::TestType* ts = ATP::lookup_testtype("rand");
	f64 time_elapseds = ATP::get_ms_from_test(*ts);


	printf("	Time Elapsed(ATP->PCG_Rand):%.*f ms, thing:%f\n", 3, time_elapsed, thing / 10000000);
	printf("	Time Elapsed(ATP->rand):%.*f ms, thing2:%f\n", 3, time_elapseds, thing2 / 10000000);

}