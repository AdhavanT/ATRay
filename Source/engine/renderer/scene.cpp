#include "scene.h"

void free_scene_memory(Scene& scene)
{
	scene.materials.clear_buffer();
	scene.planes.clear_buffer();
	scene.spheres.clear_buffer();
	for (int i = 0; i < scene.models.length; i++)
	{
		scene.models[i].data.faces_data.clear();
		scene.models[i].data.faces_vertices.clear();
		scene.models[i].data.normals.clear();
		scene.models[i].data.tex_coords.clear();
		scene.models[i].data.vertices.clear();
	}
}
