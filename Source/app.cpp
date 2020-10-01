#include "app.h"
#include "utilities/atp.h"

ATP_REGISTER(render_app);

ATP_REGISTER(prep_scene);
ATP_REGISTER(render_from_camera);


void render_app(Texture& texture)
{
	
	ATP_START(render_app);

	Camera cm;
	RenderSettings rs;
	rs.no_of_threads = get_core_count();
	rs.anti_aliasing = true;
	rs.resolution.x = texture.bmb.width;
	rs.resolution.y = texture.bmb.height;
	rs.samples_per_pixel = 64;
	rs.bounce_limit = 5;

	set_camera(cm, { 0.f,1.f,0.f }, { 0.f, 0.0f,-1.f }, rs, 1.0f);

	Scene scene;
	Material skybox;


	Sphere spr[2];
	Plane pln[2];
	Model mdl[1];
	TriangleVertices tri[1];

	tri[0].a = { -3.0f, 3.0f, -5.0f };
	tri[0].b = { -1.0f, 3.0f, -5.0f };
	tri[0].c = { -2.0f, 1.0f, -5.0f };
	mdl[0].triangles.add(tri[0]);

	spr[0].center = { 0.f,1.0f,-7.f };
	spr[0].radius = 1.0f;
	spr[0].material.color = { 0.f,0.8f,0.9f };
	spr[0].material.specularity = 0.3f;

	spr[1].center = { 2.f,1.5f,-7.f };
	spr[1].radius = 1.f;
	spr[1].material.color = { 0.9f,0.4f,0.2f };
	spr[1].material.specularity = 0.9f;


	pln[0].material.color = { 0.2f, 0.3f,0.2f };
	pln[0].material.specularity = 0.f;
	pln[0].distance = 7.f;
	pln[0].normal = { -1.f,0.f,0.f };

	//ground plane
	pln[1].material.color = { 0.9f, 0.8f,0.2f };
	pln[1].material.specularity= 0.f;
	pln[1].distance = 0.f;
	pln[1].normal = { 0.f,1.f,0.f };

	scene.models.add(mdl[0]);
	scene.planes.add(pln[0]);
	scene.planes.add(pln[1]);
	scene.spheres.add(spr[0]);
	scene.spheres.add(spr[1]);

	scene.skybox.specularity = 1.0f;
	scene.skybox.color = { 0.1f,0.1f,0.1f };

	
	ATP_START(prep_scene);
	prep_scene(scene);
	ATP_END(prep_scene);

	printf("\nResolution [%i,%i] || Samples per pixel - %i - Starting Render...\n",texture.bmb.width, texture.bmb.height, rs.samples_per_pixel);
	
	ATP_START(render_from_camera);
	int64 rays_shot = render_from_camera(cm, scene, texture);
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
