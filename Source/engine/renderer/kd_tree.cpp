#include "model.h"

FORCEDINLINE f32 area_of_triangle(TriangleVertices tri)
{
	vec3f ab = tri.a - tri.b;
	vec3f ac = tri.a - tri.c;
	return mag(cross(ac, ab))/2;
}

static void build_binary_kd_tree(KD_Tree* tree)
{
	DBuffer<uint32, 1000, 1000> node_stack;

	node_stack.add(0);


	while (node_stack.length > 0)
	{

		KD_Node* current_node = &tree->tree[node_stack[node_stack.length - 1]];
		node_stack.length--;
		if (current_node->primitives.size > tree->max_no_faces_per_node)
		{
			//find divison point 
			vec3f division_point;
			switch (tree->division_method)
			{
			case KD_Division_Method::CENTER:
			{
				division_point = (current_node->aabb.max - current_node->aabb.min) / 2; //TODO: find SAH point
				division_point += current_node->aabb.min;

				ASSERT(is_inside(division_point, current_node->aabb));	//The division point is not inside the the aabb for some reason

			}break;
			case KD_Division_Method::SAH:
			{
				//Finding center using geometric decomposition
				vec3f sum = {};
				f64 sum_of_areas = 0;
				for (uint32 i = 0; i < current_node->primitives.size; i++)
				{
					vec3f tri_center = (current_node->primitives[i].face_vertices.a + current_node->primitives[i].face_vertices.b + current_node->primitives[i].face_vertices.c) / 3;
					f32 area = area_of_triangle(current_node->primitives[i].face_vertices);
					sum += tri_center * area;
					sum_of_areas += area;
				}
				division_point = sum / (f32)sum_of_areas;

				if (!is_inside(division_point, current_node->aabb))//This occurs in SAH mode when most of the primitive areas are outside the aabb. Must resort to aborting dividing current_node and make it a leaf.
				{
					//making current_node a leaf node
					current_node->has_children = FALSE;
					continue;
				}

			}break;
			default:
				ASSERT(FALSE);	//Improper Division method
				break;
			}
			KD_Node left = {}, right = {};

			AABB& p = current_node->aabb;
			vec3f& v = division_point;
			//cuts along Z-Y plane.
			AABB& l = left.aabb, & r = right.aabb;
			l.min = p.min;
			l.max.x = v.x;
			l.max.y = p.max.y;
			l.max.z = p.max.z;

			r.min.x = v.x;
			r.min.y = p.min.y;
			r.min.z = p.min.z;
			r.max = p.max;

			//filling subnode primitives
			DBuffer<KD_Primitive, 0, 0, uint32> primitives_left, primitives_right;
			uint32 temp_buffer_cap_and_addon = (current_node->primitives.size / 2);	//using half the parent nodes primitive size as a cap and overflow addon value
			primitives_left.capacity = temp_buffer_cap_and_addon;
			primitives_left.overflow_addon = temp_buffer_cap_and_addon;
			primitives_right.capacity = temp_buffer_cap_and_addon;
			primitives_right.overflow_addon = temp_buffer_cap_and_addon;


			for (uint32 i = 0; i < current_node->primitives.size; i++)
			{

				//testing if triangle is in the left or right or both
				b8 a_left = FALSE, b_left = FALSE, c_left = FALSE;
				a_left = is_inside(current_node->primitives[i].face_vertices.a, left.aabb);
				b_left = is_inside(current_node->primitives[i].face_vertices.b, left.aabb);
				c_left = is_inside(current_node->primitives[i].face_vertices.c, left.aabb);

				b8 a_right = FALSE, b_right = FALSE, c_right = FALSE;
				a_right = is_inside(current_node->primitives[i].face_vertices.a, right.aabb);
				b_right = is_inside(current_node->primitives[i].face_vertices.b, right.aabb);
				c_right = is_inside(current_node->primitives[i].face_vertices.c, right.aabb);

				if (a_left || b_left || c_left)	//a vertex is in left aabb
				{
					primitives_left.add(current_node->primitives[i]);
				}
				if (a_right || b_right || c_right) //a vertex is in right aabb
				{
					primitives_right.add(current_node->primitives[i]);
				}
			}
			left.primitives.front = primitives_left.front;
			left.primitives.size = primitives_left.length;
			right.primitives.front = primitives_right.front;
			right.primitives.size = primitives_right.length;

			ASSERT(left.primitives.size + right.primitives.size >= current_node->primitives.size);
			current_node->primitives.clear();

			current_node->children_start_position = tree->tree.length;

			//adding left and right to the tree
			tree->tree.add_nocpy(left);
			tree->tree.add_nocpy(right);

			//adding left and right to be processed in node_stack
			node_stack.add(current_node->children_start_position);
			node_stack.add(current_node->children_start_position + 1);
		}
		else
		{
			current_node->has_children = FALSE;
		}
	}
}

void build_KD_tree(ModelData mdl, KD_Tree& tree)
{
	KD_Node root = {};
	root.aabb = get_AABB(mdl);

	//Top node primitives buffer length is known. 
	root.primitives.allocate(mdl.faces_vertices.size);
	KD_Primitive* prim = root.primitives.front;
	//filling the buffer
	for (uint32 i = 0; i < root.primitives.size; i++)
	{
		TriangleVertices tri;
		tri.a = mdl.vertices[mdl.faces_vertices[i].vertex_indices[0]];
		tri.b = mdl.vertices[mdl.faces_vertices[i].vertex_indices[1]];
		tri.c = mdl.vertices[mdl.faces_vertices[i].vertex_indices[2]];

		*prim = { tri, i };
		prim++;
	}
	if (tree.tree.length > 0)
	{
		tree.tree.clear_buffer();
	}
	tree.tree.add_nocpy(root);

	switch (tree.max_divisions)
	{
	case KD_Divisions::TWO:
	{
		//NOTE: replaced recursion with a stack of nodes.
		build_binary_kd_tree(&tree);
	}break;

	case KD_Divisions::FOUR:
	{
		//
	}break;

	case KD_Divisions::EIGHT:
	{

	}break;

	}
}

struct TraversalData
{
	Optimized_Ray* ray;
	KD_Tree* tree;
	TriangleIntersectionData* tri_data;
	f32* closest;
};

//defined in renderer.cpp

static void traverse_two_subnode_tree(TraversalData& td, KD_Node* current_node)
{
	if (!get_ray_AABB_intersection(*td.ray, current_node->aabb))
	{
		return;
	}
	if (current_node->has_children)
	{
		traverse_two_subnode_tree(td, &td.tree->tree[current_node->children_start_position]);	//left node
		traverse_two_subnode_tree(td, &td.tree->tree[current_node->children_start_position + 1]);	//right node
	}
	else   //is leaf node
	{
		KD_Primitive* prim = current_node->primitives.front;
		for (uint32 i = 0; i < current_node->primitives.size; i++)
		{
			f32 u, v;
			f32 distance = get_triangle_ray_intersection_culled(td.ray->ray, prim->face_vertices, u, v);
			if (distance < *td.closest && distance > tolerance)
			{
				*td.closest = distance;
				td.tri_data->face_index = current_node->primitives[i].face_index;
				td.tri_data->u = u;
				td.tri_data->v = v;
			}
			prim++;
		}
	}
}


f32 get_ray_kd_tree_intersection(Optimized_Ray &op_ray, KD_Tree& tree, TriangleIntersectionData& tri_data)
{
	f32 closest = MAX_FLOAT;

	TraversalData td;
	td.closest = &closest;
	td.ray = &op_ray;
	td.tri_data = &tri_data;
	td.tree = &tree;

	switch (tree.max_divisions)
	{
		case KD_Divisions::TWO:
		{
			traverse_two_subnode_tree(td, td.tree->tree.front);
			return closest;
		}break;
		default:
			ASSERT(FALSE);	//max_divisions traversal isn't defined
	}
	return 0;
}