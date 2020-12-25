#include "model.h"

FORCEDINLINE f32 area_of_triangle(TriangleVertices tri)
{
	vec3f ab = tri.a - tri.b;
	vec3f ac = tri.a - tri.c;
	return mag(cross(ac, ab))/2;
}

void create_binary_kd_tree(KD_Tree* tree_, KD_Node* current_node_)
{
	KD_Node& current_node = *current_node_;
	KD_Tree& tree = *tree_;
	if (current_node.primitives.size > (uint32)tree.max_no_faces_per_node)
	{
		current_node.children_start_position = tree.tree.length;
		KD_Node *l, *r;
		
		l = tree.tree.add({ 0 });
		r = tree.tree.add({ 0 });
		

		{
			vec3f division_point;
			switch (tree.division_method)
			{
			case KD_Division_Method::CENTER:
			{
				division_point = (current_node.aabb.max - current_node.aabb.min) / 2; //TODO: find SAH point
				division_point += current_node.aabb.min;
			}break;
			case KD_Division_Method::SAH:
			{
				vec3f sum = {};
				for (uint32 i = 0; current_node.primitives.size; i++)
				{
					vec3f tri_center = (current_node.primitives[i].face_vertices.a + current_node.primitives[i].face_vertices.b + current_node.primitives[i].face_vertices.c)/3;
					sum += tri_center * area_of_triangle(current_node.primitives[i].face_vertices);
				}
				sum = sum / (f32)current_node.primitives.size;
			}break;
			default:
				ASSERT(FALSE);	//Improper Division method
				break;
			}

			AABB& p = current_node.aabb;
			vec3f& v = division_point;
			ASSERT(is_inside(v, p));
			//cuts along Z-Y plane.
			AABB& left = l->aabb, & right = r->aabb;
			left.min = p.min;
			left.max.x = v.x;
			left.max.y = p.max.y;
			left.max.z = p.max.z;

			right.min.x = v.x;
			right.min.y = p.min.x;
			right.min.z = p.min.z;
			right.max = p.max;
		}

		//TODO: MAKE AND USE A MEMORY ARENA.
		//ASSESS: This is the place where a memory arena might be much faster. Using a DBuffer will require either a large amount of memory be allocated for each node, or lots of reallocs happening.

		{
			//filling subnode primitives
			DBuffer<KD_Primitive, 100, 100, uint32> primitives_left, primitives_right;

			for (uint32 i = 0; i < current_node.primitives.size; i++)
			{
				b32 a_left = FALSE, b_left = FALSE, c_left = FALSE;
				a_left = is_inside(current_node.primitives[i].face_vertices.a, l->aabb);
				b_left = is_inside(current_node.primitives[i].face_vertices.b, l->aabb);
				c_left = is_inside(current_node.primitives[i].face_vertices.c, l->aabb);

				b32 a_right = FALSE, b_right = FALSE, c_right = FALSE;
				a_right = is_inside(current_node.primitives[i].face_vertices.a, r->aabb);
				b_right = is_inside(current_node.primitives[i].face_vertices.b, r->aabb);
				c_right = is_inside(current_node.primitives[i].face_vertices.c, r->aabb);

				if (a_left || b_left || c_left)	//completely in left aabb
				{
					primitives_left.add(current_node.primitives[i]);
				}
				if (a_right || b_right || c_right)
				{
					primitives_right.add(current_node.primitives[i]);
				}
			}
			l->primitives.front = primitives_left.front;
			l->primitives.size = primitives_left.length;
			r->primitives.front = primitives_right.front;
			r->primitives.size = primitives_right.length;
		}

		current_node.primitives.clear();
		
		//foreach subnode, recursively call same function
		create_binary_kd_tree(tree_, l);
		create_binary_kd_tree(tree_, r);
	}
	else
	{
		current_node.has_children = FALSE;
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
	KD_Node* front = tree.tree.add_nocpy(root);

	switch (tree.max_divisions)
	{
	case KD_Divisions::TWO:
	{
		create_binary_kd_tree(&tree, front);
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
extern f32 tolerance;	

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