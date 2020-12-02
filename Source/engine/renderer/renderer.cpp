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

static inline vec3f get_reflection(vec3f incident, vec3f normal)
{
	normalize(normal);
	vec3f reflection = -(normal * (2 * dot(incident, normal))) + incident;
	return reflection;
}


struct TriangleIntersectionData
{
	Model* model;
	uint32 face_index;
	f32 u, v;
};

enum class ObjectType
{
	AABB,TRIANGLE, SPHERE, PLANE, SKYBOX
};

struct IntersectionData
{
	ObjectType type;
	f32 distance_at_intersection;
	vec3f normal;
	Material* hit_material;
	TriangleIntersectionData tid;
};

void get_intersection_data(Ray& casted_ray, Scene& scene, IntersectionData& intersection_data)
{
	f32 closest = MAX_FLOAT;
	Sphere* nearest_sphere = nullptr;
	Plane* nearest_plane = nullptr;
	intersection_data.tid.model = nullptr;


	for (int32 i = 0; i < scene.models.length; i++)
	{
		uint32 face_data_index = -1;	//assumes number of faces for model is less than 4,294,967,295 (MAX of UINT32)
		for (uint32 j = 0; j < scene.models[i].data.faces_vertices.size; j++)
		{
			TriangleVertices tri;
			tri.a = scene.models[i].data.vertices[scene.models[i].data.faces_vertices[j].vertex_indices[0]];
			tri.b = scene.models[i].data.vertices[scene.models[i].data.faces_vertices[j].vertex_indices[1]];
			tri.c = scene.models[i].data.vertices[scene.models[i].data.faces_vertices[j].vertex_indices[2]];

			f32 u, v;
			//TODO: check if passing by value or manually inlining is faster for checking intersection
			f32 t = get_triangle_ray_intersection_culled(casted_ray, tri, u, v);
			if (t > tolerance && t < closest)
			{
				closest = t;
				intersection_data.tid.u = u;
				intersection_data.tid.v = v;
				face_data_index = j;
			}
		}
		if (face_data_index != -1)	//closest intersection was found for this model
		{
			intersection_data.tid.face_index = face_data_index;
			intersection_data.tid.model = &scene.models[i];
		}
	}

	for (int i = 0; i < scene.spheres.length; i++)
	{
		Sphere* spr = &scene.spheres[i];

		f32 t = get_sphere_ray_intersection(casted_ray, *spr);
		if (t > tolerance && t < closest)
		{
			closest = t;
			nearest_sphere = spr;
		}
	}
	for (int i = 0; i < scene.planes.length; i++)
	{
		Plane* pln = &scene.planes[i];

		f32 t = get_plane_ray_intersection(casted_ray, *pln);
		if (t > tolerance && t < closest)
		{
			nearest_plane = pln;
			closest = t;
		}
	}


	//getting reflected normal
	if (nearest_plane != nullptr)	//nearest hit is a plane
	{
		intersection_data.type = ObjectType::PLANE;
		intersection_data.normal = nearest_plane->normal;
		intersection_data.hit_material = nearest_plane->material;
	}
	else if (nearest_sphere != nullptr)	//nearest hit is a sphere
	{
		intersection_data.type = ObjectType::SPHERE;
		intersection_data.normal = casted_ray.at(closest) - nearest_sphere->center;
		intersection_data.hit_material = nearest_sphere->material;
	}
	//just some code to test if intersection works
	else if (intersection_data.tid.model != nullptr)	//nearest hit is a triangle
	{
		intersection_data.type = ObjectType::TRIANGLE;
		//Smooth Shading
		vec3f normal_a, normal_b, normal_c;
		if (intersection_data.tid.model->data.normals.size > 0)
		{
			FaceData* face_data = &intersection_data.tid.model->data.faces_data[intersection_data.tid.face_index];
			normal_a = intersection_data.tid.model->data.normals[face_data->vertex_normals_indices[0]];
			normal_b = intersection_data.tid.model->data.normals[face_data->vertex_normals_indices[1]];;
			normal_c = intersection_data.tid.model->data.normals[face_data->vertex_normals_indices[2]];;

			//Interpolating normals
			intersection_data.normal = normal_a * (1 - intersection_data.tid.u - intersection_data.tid.v) + normal_b * intersection_data.tid.u + normal_c * intersection_data.tid.v;
		}
		//regular shading
		else
		{
			FaceVertices* face_v = &intersection_data.tid.model->data.faces_vertices[intersection_data.tid.face_index];
			vec3f ab = intersection_data.tid.model->data.vertices[face_v->vertex_indices[0]] - intersection_data.tid.model->data.vertices[face_v->vertex_indices[1]];
			vec3f ac = intersection_data.tid.model->data.vertices[face_v->vertex_indices[0]] - intersection_data.tid.model->data.vertices[face_v->vertex_indices[2]];
			intersection_data.normal = cross(ab, ac);
		}
		//TODO: set the material for 
		intersection_data.hit_material = intersection_data.tid.model->data.material;
	}
	else
	{
		//Hits nothing but Skybox
		intersection_data.hit_material = &scene.materials[0];	//material 0 is skybox
		intersection_data.type = ObjectType::SKYBOX;
	}
	intersection_data.distance_at_intersection = closest;
	normalize(intersection_data.normal);

	return;
}

//Quick shading using recursion
//static vec3f cast_ray(Ray& ray, Scene& scene, int32 bounce_limit, int64& ray_casts, RNG_Stream* rng_stream)
//{
//
//	if (bounce_limit <= 0)
//	{
//		return { scene.materials[0].emission_color };
//	}
//	ray_casts++;
//
//	IntersectionData id;
//
//	get_intersection_data(ray, scene, id);
//
//	if (id.type == ObjectType::SKYBOX)
//	{
//		return id.hit_material->reflection_color;	
//	}
//
//	f32 attenuation = dot(-ray.direction, id.normal);
//	if (attenuation < 0)
//	{
//		id.normal = -id.normal;
//		attenuation = -attenuation;
//	}
//
//	Ray reflected;
//
//	//reflected ray
//	vec3f pure_bounce;
//	pure_bounce = reflected.direction - (id.normal * (2 * dot(reflected.direction, id.normal)));
//	normalize(pure_bounce);
//
//	//random ray
//	vec3f random_bounce = { rand_bi(rng_stream), rand_bi(rng_stream), rand_bi(rng_stream) };
//	random_bounce += id.normal;
//	normalize(random_bounce);
//
//	//final reflected ray
//	reflected.origin = reflected.at(id.distance_at_intersection);
//	reflected.direction = lerp(random_bounce, pure_bounce, id.hit_material->scatter);
//	normalize(reflected.direction);
//
//	vec3f color = cast_ray(reflected, scene, bounce_limit - 1, ray_casts, rng_stream) * id.hit_material->scatter + id.hit_material->reflection_color * (1 - id.hit_material->scatter);
//	color = color * attenuation;
//
//	return  color;
//}


//returns color from casting ray into scene
static vec3f cast_ray(Ray& ray, Scene& scene, int32 bounce_limit, int64& ray_casts, RNG_Stream *rng_stream)
{
	int i;
	vec3f return_color = { 0,0,0 };
	vec3f weight = { 1.0f,1.0f,1.0f };
	Ray casted_ray = ray;
	IntersectionData id;


	for (i = 0; i < bounce_limit; i++)
	{
		
		get_intersection_data(casted_ray, scene, id);

		if (id.type == ObjectType::SKYBOX)
		{
			return_color += hadamard(weight , id.hit_material->emission_color);	
			break;
		}

		f32 attenuation = dot(-casted_ray.direction, id.normal);
		if (attenuation < 0)
		{
			id.normal = -id.normal;
			attenuation = 0;
		}

		//reflected ray
		vec3f pure_bounce;
		pure_bounce = casted_ray.direction - (id.normal * (2 * dot(casted_ray.direction, id.normal)));
		normalize(pure_bounce);
			
		//random ray
		vec3f random_bounce = {rand_bi(rng_stream), rand_bi(rng_stream), rand_bi(rng_stream)};
		random_bounce += id.normal;
		normalize(random_bounce);

		//final reflected ray
		casted_ray.origin = casted_ray.at(id.distance_at_intersection);
		casted_ray.direction = lerp(random_bounce, pure_bounce, id.hit_material->scatter);
		normalize(casted_ray.direction);

		//Shading
		vec3f reflection_amount = id.hit_material->reflection_color;
		return_color += hadamard(weight, id.hit_material->emission_color);
		weight = hadamard(weight, id.hit_material->reflection_color * attenuation);
	}
	ray_casts += i;
	return  return_color;
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

			vec3b pixel_color;
			vec3f flt_pixel_color;
			Ray ray = {};
			vec3f pixel_pos;

			if (info.camera->render_settings.anti_aliasing)
			{
				for (uint32 i = 0; i < info.camera->render_settings.samples_per_pixel; i++)
				{
					f32 x_off = rand_bi(rng_stream) * info.camera->half_pixel_width + film_x;
					f32 y_off = rand_bi(rng_stream) * info.camera->half_pixel_height + film_y;
					pixel_pos = info.camera->frame_center + (info.camera->camera_x * x_off) + (info.camera->camera_y * y_off);
					SetRay(ray, info.camera->eye, pixel_pos);

					flt_pixel_color += cast_ray(ray, *info.scene, info.camera->render_settings.bounce_limit, ray_casts, rng_stream);
				}
			}
			else
			{
				pixel_pos = info.camera->frame_center + (info.camera->camera_x * film_x) + (info.camera->camera_y * film_y);
				SetRay(ray, info.camera->eye, pixel_pos);

				for (uint32 i = 0; i < info.camera->render_settings.samples_per_pixel; i++)
				{
					flt_pixel_color += cast_ray(ray, *info.scene, info.camera->render_settings.bounce_limit, ray_casts, rng_stream);
				}
			}
			flt_pixel_color = flt_pixel_color / (f32)info.camera->render_settings.samples_per_pixel;

			flt_pixel_color = clamp(flt_pixel_color, 0.0f, 1.0f);
			//flt_pixel_color = rgb_gamma_correct(flt_pixel_color);
			//flt_pixel_color = linear_to_srgb(flt_pixel_color);
			pixel_color = rgb_float_to_byte(flt_pixel_color);

			Set_Pixel(pixel_color, *info.camera_tex, x, y);
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

