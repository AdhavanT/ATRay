#include "renderer.h"


f32 tolerance = 0.0001f;

vec3f cast_ray(Ray& ray, Scene& scene, int32 bounce_limit, int64& ray_casts);


//renders image only on passed tile 
static b32 render_tile_from_camera(TileWorkQueue &twq)
{
	Tile *tile_;
	int64 tile_no = interlocked_increment(&twq.current_tile);
	if (tile_no > twq.no_of_tiles )
	{
		return false;
	}
	else
	{
		tile_ = twq.tiles + tile_no - 1;
	}

	vec3f pixel_pos;
	int64 ray_casts = 0;

	for (int32 y = tile_->left_top.y; y <= tile_->right_bottom.y; y++)
	{

		f32 film_y = -1.0f + 2.0f * ((f32)y / (f32)twq.info.camera->resolution.y);

		for (int32 x = tile_->left_top.x; x <= tile_->right_bottom.x; x++)
		{
			f32 film_x = (-1.0f + 2.0f * ((f32)x / (f32)twq.info.camera->resolution.x)) * twq.info.camera->h_fov * twq.info.camera->aspect_ratio;

			vec3b pixel_color = {};
			vec3f flt_pixel_color = {};
			Ray ray = {};

			if (twq.info.camera->toggle_anti_aliasing)
			{
				vec3f AA_pixel_pos = pixel_pos;
				for (uint32 i = 0; i < twq.info.camera->samples_per_pixel; i++)
				{
					f32 x_off = rand_bi() * twq.info.camera->half_pixel_width + film_x;
					f32 y_off = rand_bi() * twq.info.camera->half_pixel_height + film_y;
					AA_pixel_pos = twq.info.camera->frame_center + (twq.info.camera->camera_x * x_off) + (twq.info.camera->camera_y * y_off);
					ray.SetRay(twq.info.camera->eye, AA_pixel_pos);
					flt_pixel_color += cast_ray(ray, *twq.info.scene, twq.info.ray_bounce_limit, ray_casts);
				}
			}
			else
			{
				pixel_pos = twq.info.camera->frame_center + (twq.info.camera->camera_x * film_x) + (twq.info.camera->camera_y * film_y);
				ray.SetRay(twq.info.camera->eye, pixel_pos);
				for (uint32 i = 0; i < twq.info.camera->samples_per_pixel; i++)
				{
					flt_pixel_color += cast_ray(ray, *twq.info.scene, twq.info.ray_bounce_limit, ray_casts);
				}
			}
			flt_pixel_color = flt_pixel_color / (f32)twq.info.camera->samples_per_pixel;
			pixel_color = rgb_float_to_byte(flt_pixel_color);
			Set_Pixel(x, y, pixel_color, *twq.info.bmb);
		}
	}

	interlocked_add(&twq.total_ray_casts, ray_casts);

	return true;
}

static void start_thread(void* data)
{
	TileWorkQueue* twq = (TileWorkQueue*)data;
	while (render_tile_from_camera(*twq));
}

//Divides image into tiles and creates multiple threads to finish all tiles. 
int64 render_from_camera(Camera& cm, Scene& scene, BitmapBuffer& bmb, int32 ray_bounce_limit)
{
	
	TileWorkQueue twq;
	
	cm.aspect_ratio = cm.resolution.x / (f32)cm.resolution.y;
	normalize(cm.facing_towards);
	cm.frame_center = cm.eye + cm.facing_towards;
	cm.camera_z = -cm.facing_towards;
	vec3f y_axis = { 0.f,1.f,0.f };
	cm.camera_x = cross(cm.camera_z, y_axis);
	normalize(cm.camera_x);
	cm.camera_y = cross(cm.camera_x, cm.camera_z);
	normalize(cm.camera_y);

	cm.half_pixel_width = (0.5f * cm.h_fov) / (f32)cm.resolution.x;
	cm.half_pixel_height = 0.5f / (f32)cm.resolution.y;


	int32 tile_width = bmb.width / get_core_count();	//ASSESS: which tile_width value gives best results
	int32 tile_height = tile_width;	// for square tiles

 	ASSERT(tile_width > 0 && tile_height > 0 && tile_height <= (int32)bmb.height);

	int32 no_x_tiles = (bmb.width + tile_width - 1) / tile_width;
	int32 no_y_tiles = (bmb.height + tile_height - 1) / tile_height;

	int32 total_tiles = no_y_tiles * no_x_tiles;

	twq.info.bmb = &bmb;
	twq.info.camera = &cm;
	twq.info.ray_bounce_limit = ray_bounce_limit;
	twq.info.scene = &scene;
	twq.no_of_tiles = total_tiles;
	twq.tiles = (Tile*)malloc(sizeof(Tile) * total_tiles);

	Tile* tmp = twq.tiles;

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
			if (maxx > bmb.width - 1)
			{
				maxx = (bmb.width - 1);
			}
			if (maxy > bmb.height - 1)
			{
				maxy = (bmb.height - 1);
			}
			tmp->left_top = { (int32)minx, (int32)miny };
			tmp->right_bottom = { (int32)maxx, (int32)maxy };
			tmp = tmp++;
		}
	}

	ThreadHandle* threads = (ThreadHandle*)malloc(sizeof(ThreadHandle) * get_core_count());
	for (uint32 i = 0; i < get_core_count(); i++)
	{
		threads[i] = create_thread(start_thread, &twq);
	}

	wait_for_all_threads(get_core_count(), threads, INFINITE);
	close_threads(get_core_count(), threads);

	return twq.total_ray_casts;
}


inline vec3f get_reflection(vec3f incident, vec3f normal)
{
	normalize(normal);
	vec3f reflection = -(normal * (2 * dot(incident, normal))) + incident;
	return reflection;
}


static inline f32 get_sphere_ray_intersection(Ray& ray, Sphere& sphere)
{
	vec3f p_to_c = (ray.origin - sphere.center);
	f32 p_to_c_sqr = mag2(p_to_c);
	f32 b = 2 * (dot(ray.direction, p_to_c));
	f32 b_sqr = sqr(b);
	f32 c = p_to_c_sqr - sqr(sphere.radius);

	f32 dmt = b_sqr - (4 * c);
	
	if (dmt < 0)
	{
		return 0;
	}
	f32 ta, tb;
	ta = (-b + sqrtf(dmt)) * 0.5f;
	tb = (-b - sqrtf(dmt)) * 0.5f;
	if (ta <= 0 && tb <= 0)
	{
		return 0;
	}
	if ( tb > 0)
	{
		return tb;
	}
	return ta;
	
}

static inline f32 get_plane_ray_intersection( Ray& ray, Plane& plane)
{
	//assumes plane normal is normalized
	f32 denom = dot(plane.normal, ray.direction);
	if (denom > -tolerance && denom < tolerance)
	{
		return 0;
	}
	f32 t = (-plane.distance - dot(ray.origin, plane.normal)) / denom;
	return t;
}


vec3f cast_ray(Ray& ray, Scene& scene, int32 bounce_limit, int64& ray_casts)
{

	vec3f skybox_color = { 0.0f,0.2f,0.4f };
	if (bounce_limit == 0)
	{
		return {0,0,0};
	}
	ray_casts++;

	f32 closest = MAX_FLOAT;
	Sphere *nearest_sphere = nullptr;
	Plane *nearest_plane = nullptr;
	//Testing for both planes and spheres. (whichever is a closer hit)

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
	Material *material;
	reflected.origin = ray.at(closest);
	vec3f hit_normal;

	//getting reflected normal
	if (nearest_plane != nullptr)	//nearest hit is a plane
	{
		hit_normal = nearest_plane->normal;
		material = &nearest_plane->material;
	}
	else if(nearest_sphere != nullptr)	//nearest hit is a sphere
	{
		hit_normal = reflected.origin - nearest_sphere->center;
		material = &nearest_sphere->material;
	}
	else
	{
		return skybox_color;
	}

	normalize(hit_normal);
	f32 attenuation = dot(ray.direction, hit_normal);
	if (attenuation < 0)
	{
		hit_normal = -hit_normal;
		attenuation = -attenuation;
	}

	reflected.direction = ray.direction -(hit_normal * (2 * attenuation));
	normalize(reflected.direction);
	
	vec3f color = cast_ray(reflected, scene, bounce_limit - 1, ray_casts) * material->specularity + material->color * (1 - material->specularity);
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
}