#include "renderer.h"
#include "camera.h"

f32 tolerance = 0.0001f;

vec3f cast_ray(Ray& ray, Scene& scene, int32 bounce_limit, int64& ray_casts, RNG_Stream* rng_stream);


b32 render_tile_from_camera(RenderInfo& info, RNG_Stream* rng_stream)
{
	Tile* tile_;
	int64 tile_no = interlocked_increment(&info.twq.current_tile);
	if (tile_no > info.twq.no_of_tiles)
	{
		interlocked_decrement(&info.twq.current_tile);
		return false;
	}
	else
	{
		tile_ = info.twq.tiles + tile_no - 1;
	}

	vec3f pixel_pos;
	int64 ray_casts = 0;

	for (int32 y = tile_->left_top.y; y <= tile_->right_bottom.y; y++)
	{

		f32 film_y = -1.0f + 2.0f * ((f32)y / (f32)info.camera->render_settings.resolution.y);

		for (int32 x = tile_->left_top.x; x <= tile_->right_bottom.x; x++)
		{
			f32 film_x = (-1.0f + 2.0f * ((f32)x / (f32)info.camera->render_settings.resolution.x)) * info.camera->h_fov * info.camera->aspect_ratio;

			vec3b pixel_color = {};
			vec3f flt_pixel_color = {};
			Ray ray = {};

			if (info.camera->render_settings.anti_aliasing)
			{
				for (uint32 i = 0; i < info.camera->render_settings.samples_per_pixel; i++)
				{
					get_ray_from_camera(ray, *info.camera, film_x, film_y, rng_stream);
					flt_pixel_color += cast_ray(ray, *info.scene, info.camera->render_settings.bounce_limit, ray_casts, rng_stream);
				}
			}
			else
			{
				get_ray_from_camera(ray, *info.camera, film_x, film_y, rng_stream);
				for (uint32 i = 0; i < info.camera->render_settings.samples_per_pixel; i++)
				{
					flt_pixel_color += cast_ray(ray, *info.scene, info.camera->render_settings.bounce_limit, ray_casts, rng_stream);
				}
			}
			flt_pixel_color = flt_pixel_color / (f32)info.camera->render_settings.samples_per_pixel;
			pixel_color = rgb_float_to_byte(flt_pixel_color);
			Set_Pixel(pixel_color,*info.tex, x, y);
		}
	}

	interlocked_add(&info.total_ray_casts, ray_casts);

	return true;
}



static void start_thread(void* data)
{
	RenderInfo* info = (RenderInfo*)data;
	RNG_Stream rng_stream;
	rng_stream.state = get_hardware_entropy();
	rng_stream.stream = (uint64)get_thread_id();

	while (render_tile_from_camera(*info, &rng_stream))
	{
		printf("\rTiles loaded: %I64i/%I64i", info->twq.current_tile, info->twq.no_of_tiles);
	}
}


//Divides image into tiles and creates multiple threads to finish all tiles. 
int64 render_from_camera(Camera& cm, Scene& scene, Texture& tex)
{
	RenderInfo info;
	info.tex = &tex;
	info.camera = &cm;
	info.scene = &scene;
	create_tile_work_queue(info.twq,tex);


	ThreadHandle* threads = (ThreadHandle*)malloc(sizeof(ThreadHandle) *cm.render_settings.no_of_threads);
	for (uint32 i = 0; i < cm.render_settings.no_of_threads; i++)
	{
		threads[i] = create_thread(start_thread, &info);
	}

	wait_for_all_threads(cm.render_settings.no_of_threads, threads, INFINITE);
	close_threads(cm.render_settings.no_of_threads, threads);

	free_tile_work_queue(info.twq);

	return info.total_ray_casts;
}


inline vec3f get_reflection(vec3f incident, vec3f normal)
{
	normalize(normal);
	vec3f reflection = -(normal * (2 * dot(incident, normal))) + incident;
	return reflection;
}


vec3f cast_ray(Ray& ray, Scene& scene, int32 bounce_limit, int64& ray_casts, RNG_Stream *rng_stream)
{

	if (bounce_limit <= 0)
	{
		return {0,0,0};
	}
	ray_casts++;

	f32 closest = MAX_FLOAT;
	Sphere *nearest_sphere = nullptr;
	Plane *nearest_plane = nullptr;
	TriangleVertices* nearest_triangle = nullptr;
	f32 u, v;

	for (int i = 0; i < scene.no_of_models; i++)
	{
		for (int j = 0; j < scene.models[i].no_of_triangles; j++)
		{
			TriangleVertices* tri = &scene.models[i].triangles[j];

			f32 t = get_triangle_ray_intersection_culled(ray,*tri,u,v);
			if (t > tolerance && t < closest)
			{
				closest = t;
				nearest_triangle = tri;
			}
		}

	}

	for (int i = 0; i < scene.no_of_spheres; i++)
	{
		Sphere* spr = &scene.spheres[i];

		f32 t = get_sphere_ray_intersection(ray, *spr);
		if (t > tolerance && t < closest)
		{
			closest = t;
			nearest_sphere = spr;
		}
	}
	for (int i = 0; i < scene.no_of_planes; i++)
	{
		Plane* pln = &scene.planes[i];

		f32 t = get_plane_ray_intersection(ray, *pln);
		if (t > tolerance && t < closest)
		{
			nearest_plane = pln;
			closest = t;
		}
	}

	Ray reflected;
	Material material;
	reflected.origin = ray.at(closest);
	vec3f hit_normal;

	//getting reflected normal
	if (nearest_plane != nullptr)	//nearest hit is a plane
	{
		hit_normal = nearest_plane->normal;
		material = nearest_plane->material;
	}
	else if(nearest_sphere != nullptr)	//nearest hit is a sphere
	{
		hit_normal = reflected.origin - nearest_sphere->center;
		normalize(hit_normal);
		material = nearest_sphere->material;
	}
	//just some code to test if intersection works
	else if (nearest_triangle != nullptr)	//nearest hit is a triangle
	{
		vec3f ab = nearest_triangle->a - nearest_triangle->b;
		vec3f ac = nearest_triangle->a - nearest_triangle->c;
		hit_normal = cross(ab, ac);
		normalize(hit_normal);
		//For now, triangles are red.
		material.color = {1,0,0};
		material.specularity = 0.2f;
	}
	else
	{
		return scene.skybox.color;
	}

	f32 attenuation = dot(ray.direction, hit_normal);
	if (attenuation < 0)
	{
		hit_normal = -hit_normal;
		attenuation = -attenuation;
	}

	reflected.direction = ray.direction -(hit_normal * (2 * attenuation));
	normalize(reflected.direction);
	
	vec3f color = cast_ray(reflected, scene, bounce_limit - 1, ray_casts, rng_stream) * material.specularity + material.color * (1 - material.specularity);
	color = color * attenuation;
	//vec3f color = cast_ray(reflected,scene,bounce_no - 1)
	
	return  color;
}

void prep_scene(Scene scene)
{
	for (int32 i = 0; i < scene.no_of_planes; i++)
	{
		normalize(scene.planes[i].normal);
	}

	/*for (int32 i = 0; i < scene.no_of_models; i++)
	{
		for (int32 j = 0; j < scene.models[i].no_of_triangles; j++)
		{
			vec3f ab = scene.models[i].triangles[j].a - scene.models[i].triangles[j].b;
			vec3f ac = scene.models[i].triangles[j].a - scene.models[i].triangles[j].c;
			scene.triangles[i].normal = cross(ab, ac);
			normalize(scene.triangles[i].normal);
		}
	}*/
}