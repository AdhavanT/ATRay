#include "renderer.h"
#include "utilities/ATP/atp.h"

static FORCEDINLINE vec3f get_reflection(vec3f incident, vec3f normal)
{
	normalize(normal);
	vec3f reflection = -(normal * (2 * dot(incident, normal))) + incident;
	return reflection;
}


enum class ObjectType
{
	NONE,TRIANGLE, SPHERE, PLANE, SKYBOX
};

struct IntersectionData
{
	ObjectType type = {};
	f32 distance_at_intersection = 0;
	vec3f normal = {0};
	Material* hit_material = 0;

	TriangleIntersectionData tid = { 0 };
};

struct RayCastTools
{
	RNG_Stream* rng_stream;
	DBuffer<KD_Node*>* hit_stack;
	DBuffer<LeafNodePair>* leaf_stack;
};

void get_intersection_data(Ray& casted_ray, Scene& scene, IntersectionData& intersection_data, RayCastTools& tools)
{
	intersection_data.distance_at_intersection = MAX_FLOAT;
	Sphere* nearest_sphere = nullptr;
	Plane* nearest_plane = nullptr;
	Model* nearest_model = nullptr;

	Optimized_Ray op_ray;
	op_ray.ray = casted_ray;
	op_ray.inv_ray_d = { 1 / casted_ray.direction.x,1 / casted_ray.direction.y, 1 / casted_ray.direction.z };
	op_ray.inv_signs = { op_ray.inv_ray_d.x < 0, op_ray.inv_ray_d.y < 0, op_ray.inv_ray_d.z < 0 };

	
	for (int32 i = 0; i < scene.models.length; i++)
	{
#if defined(USE_KD_TREE)
		TriangleIntersectionData td;
		f32 t = get_ray_kd_tree_intersection(op_ray, scene.models[i].kd_tree, td, tools.hit_stack->front, tools.leaf_stack->front);
		if (t > tolerance && t < intersection_data.distance_at_intersection)
		{
			intersection_data.distance_at_intersection = t;
			intersection_data.tid = td;
			nearest_model = &scene.models[i];
		}
#else
		if (get_ray_AABB_intersection(op_ray, scene.models[i].surrounding_aabb))
		{
			for (uint32 j = 0; j < scene.models[i].data.faces_vertices.size; j++)
			{
				TriangleVertices tri;
				tri.a = scene.models[i].data.vertices[scene.models[i].data.faces_vertices[j].vertex_indices[0]];
				tri.b = scene.models[i].data.vertices[scene.models[i].data.faces_vertices[j].vertex_indices[1]];
				tri.c = scene.models[i].data.vertices[scene.models[i].data.faces_vertices[j].vertex_indices[2]];

				f32 u, v;
				//TODO: check if passing by value or manually inlining is faster for checking intersection
				f32 t = get_triangle_ray_intersection_culled(casted_ray, tri, u, v);
				if (t > tolerance && t < intersection_data.distance_at_intersection)
				{
					intersection_data.distance_at_intersection = t;
					intersection_data.tid.u = u;
					intersection_data.tid.v = v;
					intersection_data.tid.face_index = j;
					nearest_model = &scene.models[i];
				}
			}
			
		}		
#endif

	}

	for (int i = 0; i < scene.spheres.length; i++)
	{
		Sphere* spr = &scene.spheres[i];

		f32 t = get_sphere_ray_intersection(casted_ray, *spr);
		if (t > tolerance && t < intersection_data.distance_at_intersection)
		{
			intersection_data.distance_at_intersection = t;
			nearest_sphere = spr;
		}
	}
	for (int i = 0; i < scene.planes.length; i++)
	{
		Plane* pln = &scene.planes[i];

		f32 t = get_plane_ray_intersection(casted_ray, *pln);
		if (t > tolerance && t < intersection_data.distance_at_intersection)
		{
			nearest_plane = pln;
			intersection_data.distance_at_intersection = t;
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
		intersection_data.normal = casted_ray.at(intersection_data.distance_at_intersection) - nearest_sphere->center;
		intersection_data.hit_material = nearest_sphere->material;
	}
	//just some code to test if intersection works
	else if (nearest_model != nullptr)	//nearest hit is a triangle
	{
		intersection_data.type = ObjectType::TRIANGLE;
		//Smooth Shading
		vec3f normal_a, normal_b, normal_c;
		if (nearest_model->data.normals.size > 0)
		{
			FaceData* face_data = &nearest_model->data.faces_data[intersection_data.tid.face_index];
			normal_a = nearest_model->data.normals[face_data->vertex_normals_indices[0]];
			normal_b = nearest_model->data.normals[face_data->vertex_normals_indices[1]];;
			normal_c = nearest_model->data.normals[face_data->vertex_normals_indices[2]];;

			//Interpolating normals
			intersection_data.normal = normal_a * (1 - intersection_data.tid.u - intersection_data.tid.v) + normal_b * intersection_data.tid.u + normal_c * intersection_data.tid.v;
		}
		//regular shading
		else
		{
			FaceVertices* face_v = &nearest_model->data.faces_vertices[intersection_data.tid.face_index];
			vec3f ab = nearest_model->data.vertices[face_v->vertex_indices[0]] - nearest_model->data.vertices[face_v->vertex_indices[1]];
			vec3f ac = nearest_model->data.vertices[face_v->vertex_indices[0]] - nearest_model->data.vertices[face_v->vertex_indices[2]];
			intersection_data.normal = cross(ab, ac);
		}
		//TODO: set the material for 
		intersection_data.hit_material = nearest_model->data.material;
	}
	else
	{
		//Hits nothing but Skybox
		//TODO: proper skybox intersection. Maybe cube map
		intersection_data.hit_material = &scene.materials[0];	//material 0 is skybox
		intersection_data.type = ObjectType::SKYBOX;
	}
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
static vec3f cast_ray(Ray& ray, Scene& scene, int32 bounce_limit, int64& ray_casts, RayCastTools& tools )
{
	int i;
	vec3f return_color = { 0,0,0 };
	vec3f weight = { 1.0f,1.0f,1.0f };
	Ray casted_ray = ray;
	IntersectionData id;


	for (i = 0; i < bounce_limit; i++)
	{
		
		get_intersection_data(casted_ray, scene, id, tools);

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
		vec3f random_bounce = {rand_bi(tools.rng_stream), rand_bi(tools.rng_stream), rand_bi(tools.rng_stream)};
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

void prep_scene(Scene &scene, uint32& kd_tree_max_nodes)
{
	kd_tree_max_nodes = 0;
	for (int32 i = 0; i < scene.planes.length; i++)
	{
		normalize(scene.planes[i].normal);
	}
	for (int32 i = 0; i < scene.models.length; i++)
	{
#if defined USE_KD_TREE
		if (scene.models[i].kd_tree.tree.front == 0)
		{
			build_KD_tree(scene.models[i].data, scene.models[i].kd_tree);
			if (scene.models[i].data.normals.size > 0)
			{
				scene.models[i].data.faces_vertices.clear();
				scene.models[i].data.vertices.clear();
			}
		}
		if (scene.models[i].kd_tree.tree.length > (int32)kd_tree_max_nodes)
		{
			kd_tree_max_nodes = scene.models[i].kd_tree.tree.length;
		}
#else

#endif
	}
}

ATP_REGISTER_M(Tiles, 0);
static b32 render_tile_from_camera(RenderInfo& info, RayCastTools& tools)
{
	RenderTile* rt;
	Tile* tile_;
	int64 tile_no = interlocked_increment_i32(&info.twq.jobs_done);
	if (tile_no > info.twq.jobs.size)
	{
		interlocked_decrement_i32(&info.twq.jobs_done);
		return false;
	}
	else
	{
		rt = &info.twq.jobs[(int32)tile_no - 1];
	}
	ATP_BLOCK_M(Tiles, (uint32)tile_no-1);


	tile_ = &rt->tile;
	vec3f pixel_pos;

	for (int32 y = tile_->left_bottom.y; y <= tile_->right_top.y; y++)
	{

		f32 film_y = -1.0f + 2.0f * ((f32)y / (f32)info.camera->render_settings.resolution.y);

		for (int32 x = tile_->left_bottom.x; x <= tile_->right_top.x; x++)
		{
			/*if (x == 645 && y == 452)	//to debug a single pixel
			{
				__debugbreak();
			}
			else
			{
				continue;
			}*/
			f32 film_x = (-1.0f + 2.0f * ((f32)x / (f32)info.camera->render_settings.resolution.x)) * info.camera->h_fov * info.camera->aspect_ratio;

			vec3b pixel_color;
			vec3f flt_pixel_color;
			Ray ray = {};
			vec3f pixel_pos;

			if (info.camera->render_settings.anti_aliasing)
			{
				for (uint32 i = 0; i < info.camera->render_settings.samples_per_pixel; i++)
				{
					f32 x_off = rand_bi(tools.rng_stream) * info.camera->half_pixel_width + film_x;
					f32 y_off = rand_bi(tools.rng_stream) * info.camera->half_pixel_height + film_y;
					pixel_pos = info.camera->frame_center + (info.camera->camera_x * x_off) + (info.camera->camera_y * y_off);
					SetRay(ray, info.camera->eye, pixel_pos);

					flt_pixel_color += cast_ray(ray, *info.scene, info.camera->render_settings.bounce_limit, rt->ray_casts, tools);
				}
			}
			else
			{
				pixel_pos = info.camera->frame_center + (info.camera->camera_x * film_x) + (info.camera->camera_y * film_y);
				SetRay(ray, info.camera->eye, pixel_pos);

				for (uint32 i = 0; i < info.camera->render_settings.samples_per_pixel; i++)
				{
					flt_pixel_color += cast_ray(ray, *info.scene, info.camera->render_settings.bounce_limit, rt->ray_casts, tools);
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
	return true;
}

static void start_tile_render_thread(void* data)
{
	RenderInfo* info = (RenderInfo*)data;
	

	RNG_Stream rng_stream;
	rng_stream.state = pl_get_hardware_entropy();
	rng_stream.stream = (uint64)pl_get_thread_id();

	DBuffer<KD_Node*> hit_stack;	//a list of non-leaf nodes the ray hits and needs to traverse for KD traversal
	DBuffer<LeafNodePair> leaf_stack;	//a list of leaf nodes the ray hits for KD traversal
	hit_stack.capacity = info->hit_stack_capacity;
	leaf_stack.capacity = info->leaf_stack_capacity;
	hit_stack.front = (KD_Node**)pl_buffer_alloc(hit_stack.capacity * sizeof(KD_Node*));
	leaf_stack.front = (LeafNodePair*)pl_buffer_alloc(leaf_stack.capacity + 1 * sizeof(LeafNodePair));
	*leaf_stack.front = { 0,-MAX_FLOAT };	//used as barrier in KD_traversal
	leaf_stack.front++;

	RayCastTools tools;
	tools.rng_stream = &rng_stream;
	tools.leaf_stack = &leaf_stack;
	tools.hit_stack = &hit_stack;
	
	while (render_tile_from_camera(*info, tools));

	leaf_stack.front--;
	leaf_stack.clear_buffer();
	hit_stack.clear_buffer();
}


//Divides image into tiles and creates multiple threads to finish all tiles. 
void start_render_from_camera(RenderInfo& info, ThreadPool& tpool)
{
	//Creates a WorkQueue made of RenderTiles
	int32 tile_width = info.camera_tex->bmb.width / tpool.threads.size;	//ASSESS: which tile_width value gives best results
	if (tile_width > (int32)info.camera_tex->bmb.height)
	{
		tile_width = info.camera_tex->bmb.height / tpool.threads.size;
	}
	int32 tile_height = tile_width;	// for square tiles

	ASSERT(tile_width > 0 && tile_height > 0 && tile_height <= (int32)info.camera_tex->bmb.height);

	int32 no_x_tiles = (info.camera_tex->bmb.width + tile_width - 1) / tile_width;
	int32 no_y_tiles = (info.camera_tex->bmb.height + tile_height - 1) / tile_height;

	int32 total_tiles = no_y_tiles * no_x_tiles;

	RenderTile* tmp = info.twq.jobs.allocate(total_tiles);
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
			if (maxx > info.camera_tex->bmb.width - 1)
			{
				maxx = (info.camera_tex->bmb.width - 1);
			}
			if (maxy > info.camera_tex->bmb.height - 1)
			{
				maxy = (info.camera_tex->bmb.height - 1);
			}
			tmp->tile.left_bottom = { (int32)minx, (int32)miny };
			tmp->tile.right_top = { (int32)maxx, (int32)maxy };
			tmp++;
		}
	}
	
	//----for ATP profiling----
	ATP_GET_TESTTYPE(Tiles)->tests.size = info.twq.jobs.size;
	ATP_GET_TESTTYPE(Tiles)->tests.finished_tests = 0;
	ATP_GET_TESTTYPE(Tiles)->tests.front = (ATP::TestInfo*)pl_buffer_alloc(ATP_GET_TESTTYPE(Tiles)->tests.size * sizeof(ATP::TestInfo));


	activate_pool(tpool, start_tile_render_thread, &info);
	
}

b32 wait_for_render_from_camera_to_finish(RenderInfo& info,ThreadPool& tpool, uint32 ms_to_wait_for)
{
	if (pl_wait_for_all_threads(tpool.threads.size, &tpool.threads[0].handle, ms_to_wait_for))
	{
		return TRUE;
	}
	else
	{
		for (int i = 0; i < info.twq.jobs.size; i++)
		{
			info.total_ray_casts += info.twq.jobs[i].ray_casts;
		}
		return FALSE;
	}
}