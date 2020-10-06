#include "renderer.h"

struct RenderInfo
{
	WorkQueue<Tile> twq;

	Camera* camera;
	Scene* scene;
	Texture* camera_tex;

	volatile int64 total_ray_casts = 0;
};

f32 tolerance = 0.0001f;

static vec3f cast_ray(Ray& ray, Scene& scene, int32 bounce_limit, int64& ray_casts, RNG_Stream* rng_stream);


static b32 render_tile_from_camera(RenderInfo& info, RNG_Stream* rng_stream)
{
	Tile* tile_;
	int64 tile_no = interlocked_increment_i32(&info.twq.jobs_done);
	if (tile_no > info.twq.jobs.size)
	{
		interlocked_decrement_i32(&info.twq.jobs_done);
		return false;
	}
	else
	{
		tile_ = &info.twq.jobs[(int32)tile_no - 1];
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
			Set_Pixel(pixel_color,*info.camera_tex, x, y);
		}
	}

	interlocked_add_i64(&info.total_ray_casts, ray_casts);

	return true;
}



static void start_tile_render_thread(void* data)
{
	RenderInfo* info = (RenderInfo*)data;
	RNG_Stream rng_stream;
	rng_stream.state = get_hardware_entropy();
	rng_stream.stream = (uint64)get_thread_id();

	while (render_tile_from_camera(*info, &rng_stream))
	{
		printf("\rTiles loaded: %i/%i", info->twq.jobs_done, info->twq.jobs.size);
	}
}


//Divides image into tiles and creates multiple threads to finish all tiles. 
int64 render_from_camera(Camera& cm, Scene& scene, Texture& tex, ThreadPool& tpool)
{
	RenderInfo info;
	info.camera_tex = &tex;
	info.camera = &cm;
	info.scene = &scene;


	//Creates a WorkQueue made of tiles
	int32 tile_width = tex.bmb.width / get_core_count();	//ASSESS: which tile_width value gives best results
	int32 tile_height = tile_width;	// for square tiles

	ASSERT(tile_width > 0 && tile_height > 0 && tile_height <= (int32)tex.bmb.height);

	int32 no_x_tiles = (tex.bmb.width + tile_width - 1) / tile_width;
	int32 no_y_tiles = (tex.bmb.height + tile_height - 1) / tile_height;

	int32 total_tiles = no_y_tiles * no_x_tiles;

	Tile* tmp = info.twq.jobs.allocate(total_tiles);
	info.twq.jobs_done = 0;

	//Creates the "tiles" and adds it into a "tile_work_queue"
	for (int y = 0; y < no_y_tiles; y++)
	{
		for (int x = 0; x < no_x_tiles; x++)
		{
			uint32 minx, miny, maxx, maxy;
			minx = x * tile_width;
			miny = y * tile_height;
			maxx = minx + tile_width;
			maxy = miny + tile_height;
			if (maxx > tex.bmb.width - 1)
			{
				maxx = (tex.bmb.width - 1);
			}
			if (maxy > tex.bmb.height - 1)
			{
				maxy = (tex.bmb.height - 1);
			}
			tmp->left_top = { (int32)minx, (int32)miny };
			tmp->right_bottom = { (int32)maxx, (int32)maxy };
			tmp++;
		}
	}

	activate_pool(tpool, start_tile_render_thread, &info);

	wait_for_pool(tpool);

	if (info.twq.jobs_done == info.twq.jobs.size)
	{
		info.twq.jobs.clear();
	}
	else
	{
		ASSERT(false);	//All tiles not rendered!
	}

	return info.total_ray_casts;
}


static inline vec3f get_reflection(vec3f incident, vec3f normal)
{
	normalize(normal);
	vec3f reflection = -(normal * (2 * dot(incident, normal))) + incident;
	return reflection;
}

struct TriangleIntersectionData
{
	Model* model;
	Face* face;
	f32 u, v;
};

static vec3f cast_ray(Ray& ray, Scene& scene, int32 bounce_limit, int64& ray_casts, RNG_Stream *rng_stream)
{

	if (bounce_limit <= 0)
	{
		return {0,0,0};
	}
	ray_casts++;

	f32 closest = MAX_FLOAT;
	Sphere *nearest_sphere = nullptr;
	Plane *nearest_plane = nullptr;
	TriangleIntersectionData nearest_triangle = { 0 };

	for (int32 i = 0; i < scene.models.length; i++)
	{
		for (uint32 j = 0; j < scene.models[i].data.faces.size; j++)
		{
			TriangleVertices tri;
			tri.a = scene.models[i].data.vertices[scene.models[i].data.faces[j].vertex_indices[0]];
			tri.b = scene.models[i].data.vertices[scene.models[i].data.faces[j].vertex_indices[1]];
			tri.c = scene.models[i].data.vertices[scene.models[i].data.faces[j].vertex_indices[2]];

			f32 u, v;
			f32 t = get_triangle_ray_intersection_culled(ray,tri,u,v);
			if (t > tolerance && t < closest)
			{
				closest = t;
				nearest_triangle.u = u;
				nearest_triangle.v = v;
				nearest_triangle.model = &scene.models[i];
				nearest_triangle.face = &scene.models[i].data.faces[j];
			}
		}

	}

	for (int i = 0; i < scene.spheres.length; i++)
	{
		Sphere* spr = &scene.spheres[i];

		f32 t = get_sphere_ray_intersection(ray, *spr);
		if (t > tolerance && t < closest)
		{
			closest = t;
			nearest_sphere = spr;
		}
	}
	for (int i = 0; i < scene.planes.length; i++)
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
		material = *nearest_plane->material;
	}
	else if(nearest_sphere != nullptr)	//nearest hit is a sphere
	{
		hit_normal = reflected.origin - nearest_sphere->center;
		normalize(hit_normal);
		material = *nearest_sphere->material;
	}
	//just some code to test if intersection works
	else if (nearest_triangle.model != nullptr)	//nearest hit is a triangle
	{
		vec3f normal_a, normal_b, normal_c;
		if (nearest_triangle.model->data.normals.size > 0)
		{
			normal_a = nearest_triangle.model->data.normals[nearest_triangle.face->vertex_normals_indices[0]];
			normal_b = nearest_triangle.model->data.normals[nearest_triangle.face->vertex_normals_indices[1]];
			normal_c = nearest_triangle.model->data.normals[nearest_triangle.face->vertex_normals_indices[2]];

			//Interpolating normals
			hit_normal = normal_a * (1 - nearest_triangle.u - nearest_triangle.v)  + normal_b * nearest_triangle.u + normal_c * nearest_triangle.v;
		}
		else
		{
			vec3f ab = nearest_triangle.model->data.vertices[nearest_triangle.face->vertex_indices[0]] - nearest_triangle.model->data.vertices[nearest_triangle.face->vertex_indices[1]];
			vec3f ac = nearest_triangle.model->data.vertices[nearest_triangle.face->vertex_indices[0]] - nearest_triangle.model->data.vertices[nearest_triangle.face->vertex_indices[2]];
			hit_normal = cross(ab, ac);
			normalize(hit_normal);
		}
		//For now, triangles are red.
		material.color = {1,0,0};
		material.specularity = 0.2f;
	}
	else
	{
		return scene.materials[0].color;
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
	for (int32 i = 0; i < scene.planes.length; i++)
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