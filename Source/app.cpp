#include "app.h"
#include "custom headers/atp.h"

ATP_REGISTER(render_app);

void render_app(BitmapBuffer& bmb)
{
	ATP_BLOCK(render_app);

	printf("\nResolution [%i,%i] - Starting Render...\n", bmb.width, bmb.height);

	Camera cm;
	cm.eye = { 0.f,3.f,0.f };
	cm.facing_towards = { 0.f,-0.3f,-1.f };
	cm.h_fov = 1.f;
	cm.resolution.x = bmb.width;
	cm.resolution.y = bmb.height;
	cm.toggle_anti_aliasing = true;
	cm.samples_per_pixel = 128;

	LS_Point lsp[2];
	Sphere spr[2];
	Plane pln[1];

	lsp[0].position = { -2.f, -2.f, -2.f };
	lsp[0].color = { 1.f,0.f,0.f };

	lsp[1].position = { 2.f, 1.f, -2.f };
	lsp[1].color = { 0.f,1.f,0.f };

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
	scene.no_of_LS_points = ArrayCount(lsp);
	scene.no_of_planes = ArrayCount(pln);
	scene.no_of_spheres = ArrayCount(spr);
	scene.ls_points = &lsp[0];
	scene.spheres = &spr[0];
	scene.planes = &pln[0];

	int64 rays_shot = render_from_camera(cm, scene, bmb, 5);

	printf("\nCompleted:\n");
	printf("\n Total Rays Shot: %I64i\n", rays_shot);

}

