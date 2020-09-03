#include "renderer.h"

f32 tolerance = 0.0001f;
uint64 bounces = 0;


//renders image only on passed tile 
void render_tile_from_camera(Camera& cm, Tile& tile_, Scene& scene, BitmapBuffer& bmb, int32 ray_bounce_limit)
{
	//ASSESS: take out this calculation and push into camera struct so 
	//it doesn't need to be calculated every tile
	f32 aspect_ratio = cm.resolution.x / (f32)cm.resolution.y;
	normalize(cm.facing_towards);
	vec3f frame_center = cm.eye + cm.facing_towards;
	vec3f camera_z = -cm.facing_towards;
	vec3f y_axis = { 0.f,1.f,0.f };
	vec3f camera_x = cross(camera_z, y_axis);
	normalize(camera_x);
	vec3f camera_y = cross(camera_x, camera_z);
	normalize(camera_y);

	f32 half_pixel_width = (0.5f * cm.h_fov) / (f32)cm.resolution.x;
	f32 half_pixel_height = 0.5f / (f32)cm.resolution.y;
	vec3f pixel_pos;

	for (int32 y = tile_.left_top.y; y <= tile_.right_bottom.y; y++)
	{

		f32 film_y = -1.0f + 2.0f * ((f32)y / (f32)cm.resolution.y);

		for (int32 x = tile_.left_top.x; x <= tile_.right_bottom.x; x++)
		{
			f32 film_x = (-1.0f + 2.0f * ((f32)x / (f32)cm.resolution.x)) * cm.h_fov * aspect_ratio;

			vec3b pixel_color = {};
			vec3f flt_pixel_color = {};
			Ray ray = {};

			if (cm.toggle_anti_aliasing)
			{
				vec3f AA_pixel_pos = pixel_pos;
				for (uint32 i = 0; i < cm.samples_per_pixel; i++)
				{
					f32 x_off = rand_bi() * half_pixel_width + film_x;
					f32 y_off = rand_bi() * half_pixel_height + film_y;
					AA_pixel_pos = frame_center + (camera_x * x_off) + (camera_y * y_off);
					ray.SetRay(cm.eye, AA_pixel_pos);
					flt_pixel_color += cast_ray(ray, scene, ray_bounce_limit);
				}
			}
			else
			{
				pixel_pos = frame_center + (camera_x * film_x) + (camera_y * film_y);
				ray.SetRay(cm.eye, pixel_pos);
				for (uint32 i = 0; i < cm.samples_per_pixel; i++)
				{
					flt_pixel_color += cast_ray(ray, scene, ray_bounce_limit);
				}
			}
			flt_pixel_color = flt_pixel_color / (f32)cm.samples_per_pixel;
			pixel_color = rgb_float_to_byte(flt_pixel_color);
			Set_Pixel(x, y, pixel_color, bmb);
		}
	}

}


//Divides image into tiles and sets one thread for each tile. Tiles don't exceed no_of_tiles
//ASSESS: whether dividing tiles vertically is better than horizontal or both
void render_from_camera(Camera& cm, int32 no_of_tiles, Scene& scene, BitmapBuffer& bmb, int32 ray_bounce_limit)
{
	//Divides image Horizontally

	int32 last_tile_remainder = 0;
	int32 y_offset = (int32)bmb.height / (int32)no_of_tiles;
	if (y_offset * no_of_tiles < (int32)bmb.height)
	{
		y_offset++;
		last_tile_remainder = (int32)bmb.height % y_offset;
	}

	for ( int32 i = 0; i < no_of_tiles -1; i++)
	{
		Tile tile;
		tile.left_top = { 0, i * y_offset };
		tile.right_bottom = { (int32)bmb.width - 1, ((i + 1) * y_offset) - 1 };
		render_tile_from_camera(cm, tile, scene, bmb, ray_bounce_limit);
		printf("Tile %i rendered \n", i + 1);

	}

	// Remainder tile 
	if(last_tile_remainder > 0)
	{
		printf("remainder is: %i...  rendered\n", last_tile_remainder);

		Tile tile;
		tile.left_top = { 0, (no_of_tiles - 1) * y_offset };
		tile.right_bottom = { (int32)bmb.width - 1, (int32)bmb.height - 1 };
		render_tile_from_camera(cm, tile, scene, bmb, ray_bounce_limit);
	}
	else
	{
		Tile tile;
		tile.left_top = { 0, (no_of_tiles - 1) * y_offset };
		tile.right_bottom = { (int32)bmb.width - 1, (((no_of_tiles - 1) + 1) * y_offset) - 1 };
		render_tile_from_camera(cm, tile, scene, bmb, ray_bounce_limit);
		printf("Tile rendered: %i\n", (no_of_tiles - 1));
	}
}


inline vec3f get_reflection(vec3f incident, vec3f normal)
{
	normalize(normal);
	vec3f reflection = -(normal * (2 * dot(incident, normal))) + incident;
	return reflection;
}

inline vec3f get_sphere_rand_direction();//TODO: make this i guess

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

//function that returns a pointer to nearest_sphere (null if none hit) and returns distance from ray
static inline f32 get_nearest_hit_sphere(Ray& ray,Sphere* spheres, int32 no_of_spheres, Sphere**nearest_sphere)
{
	f32 closest = MAX_FLOAT;	//distance to nearest sphere
	for (int i = 0; i < no_of_spheres; i++)
	{
		Sphere* spr = &spheres[i];
		f32 t = get_sphere_ray_intersection(ray, *spr);
		if (t > 0 && t < closest)
		{
			closest = t;
			*nearest_sphere = spr;
		}
	}

	return closest;
}

static inline f32 get_plane_ray_intersection( Ray& ray, Plane& plane)
{
	vec3f pln_normal_normalized = plane.normal;
	normalize(pln_normal_normalized);
	f32 denom = dot(pln_normal_normalized, ray.direction);
	if (denom > -tolerance && denom < tolerance)
	{
		return 0;
	}
	f32 t = (-plane.distance - dot(ray.origin, pln_normal_normalized)) / denom;
	return t;
}


static inline f32 get_luminance_from_light_sources(vec3f& point, vec3f& normal, Scene& scene)
{
	LS_Point* lsp = scene.ls_points;

	for (int i = 0; i < scene.no_of_LS_points; i++)
	{

	}
}

#include "custom headers/atp.h"
ATP_REGISTER(cast_ray);

vec3f cast_ray(Ray& ray, Scene& scene, int32 bounce_no)
{
	ATP_BLOCK(cast_ray);

	vec3f skybox_color = { 0.0f,0.2f,0.4f };
	if (bounce_no == 0)
	{
		return {0,0,0};
	}
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
	Material material;
	material.color = { 0.5,0,0 };
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
		material = nearest_sphere->material;
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

	bounces++;	

	
	reflected.direction = ray.direction -(hit_normal * (2 * attenuation));
	normalize(reflected.direction);
	
	vec3f color = material.color * attenuation;
	//vec3f color = cast_ray(reflected,scene,bounce_no - 1)
	
	


	return  color;
}