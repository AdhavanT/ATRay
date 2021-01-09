#include "scene.h"

void free_scene_memory(Scene& scene)
{
	scene.materials.clear_buffer();
	scene.planes.clear_buffer();
	scene.spheres.clear_buffer();
	for (int i = 0; i < scene.models.length; i++)
	{
		scene.models[i].data.faces_data.clear();
		if (scene.models[i].data.faces_vertices.size != 0)
		{
			scene.models[i].data.faces_vertices.clear();
		}
		scene.models[i].data.normals.clear();
		scene.models[i].data.tex_coords.clear();
		scene.models[i].data.vertices.clear();
		if (scene.models[i].kd_tree.tree.front != 0)
		{
			for (int32 j = 0; j < scene.models[i].kd_tree.tree.length; j++)
			{
				if (!scene.models[i].kd_tree.tree[j].has_children)
				{
					scene.models[i].kd_tree.tree[j].primitives.clear();
				}
			}
			scene.models[i].kd_tree.tree.clear_buffer();
		}
	}
}
