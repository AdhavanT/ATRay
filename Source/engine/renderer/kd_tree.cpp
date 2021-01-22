#include "model.h"

FORCEDINLINE f32 area_of_triangle(TriangleVertices tri)
{
	vec3f ab = tri.a - tri.b;
	vec3f ac = tri.a - tri.c;
	return mag(cross(ac, ab))/2;
}

FORCEDINLINE b8 is_inside(TriangleVertices t, AABB box)
{
	b8 a = FALSE, b = FALSE, c = FALSE;
	a = is_inside(t.a, box);
	b = is_inside(t.b, box);
	c = is_inside(t.c, box);
	return (a || b || c);
}
static void build_oct_kd_tree(KD_Tree* tree);

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

	build_oct_kd_tree(&tree);
	
	//switch (tree.max_divisions)
	//{
	//case KD_Divisions::TWO:
	//{
	//	//NOTE: replaced recursion with a stack of nodes.
	//	build_binary_kd_tree(&tree);
	//}break;

	//case KD_Divisions::FOUR:
	//{
	//	build_quad_kd_tree(&tree);
	//}break;

	//case KD_Divisions::EIGHT:
	//{
	//}break;
	//}
}


static void build_oct_kd_tree(KD_Tree* tree)
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

			KD_Node bb_left = {}, bf_left = {}, tb_left = {}, tf_left = {}, bb_right = {}, bf_right = {}, tb_right = {}, tf_right = {};

			AABB& p = current_node->aabb;
			vec3f& v = division_point;
			//cuts along Z-Y plane.

			bb_left.aabb.min = p.min;
			bb_left.aabb.max = v;

			bf_left.aabb.min = { p.min.x,p.min.y,v.z };
			bf_left.aabb.max = { v.x, v.y, p.max.z};

			tb_left.aabb.min = { p.min.x, v.y, p.min.z };
			tb_left.aabb.max = { v.x,p.max.y,v.z };

			tf_left.aabb.min = { p.min.x, v.y,v.z };
			tf_left.aabb.max = { v.x,p.max.y, p.max.z};

			bb_right.aabb.min = { v.x,p.min.y,p.min.z };
			bb_right.aabb.max = { p.max.x,v.y,v.z};

			bf_right.aabb.min = { v.x,p.min.y,v.z };
			bf_right.aabb.max = { p.max.x,v.y,p.max.z};

			tb_right.aabb.min = { v.x,v.y,p.min.z };
			tb_right.aabb.max = { p.max.x, p.max.y, v.z };

			tf_right.aabb.min = v;
			tf_right.aabb.max = p.max;


			//filling subnode primitives
			DBuffer<KD_Primitive, 0, 0, uint32> prim_bbleft, prim_bfleft, prim_tbleft, prim_tfleft, prim_bbright, prim_bfright, prim_tbright, prim_tfright;
			uint32 temp_buffer_cap_and_addon = (current_node->primitives.size / 8);	//using an 8th of the parent nodes primitive size as a cap and overflow addon value
			prim_bbleft.capacity = temp_buffer_cap_and_addon;
			prim_bbleft.overflow_addon = temp_buffer_cap_and_addon;

			prim_bfleft.capacity = temp_buffer_cap_and_addon;
			prim_bfleft.overflow_addon = temp_buffer_cap_and_addon;

			prim_tbleft.capacity = temp_buffer_cap_and_addon;
			prim_tbleft.overflow_addon = temp_buffer_cap_and_addon;

			prim_tfleft.capacity = temp_buffer_cap_and_addon;
			prim_tfleft.overflow_addon = temp_buffer_cap_and_addon;

			prim_bbright.capacity = temp_buffer_cap_and_addon;
			prim_bbright.overflow_addon = temp_buffer_cap_and_addon;
				   
			prim_bfright.capacity = temp_buffer_cap_and_addon;
			prim_bfright.overflow_addon = temp_buffer_cap_and_addon;
				   
			prim_tbright.capacity = temp_buffer_cap_and_addon;
			prim_tbright.overflow_addon = temp_buffer_cap_and_addon;
				   
			prim_tfright.capacity = temp_buffer_cap_and_addon;
			prim_tfright.overflow_addon = temp_buffer_cap_and_addon;




			for (uint32 i = 0; i < current_node->primitives.size; i++)
			{
				//ASSESS: Maybe a faster way of doin this is a loop that checks if any of the vertices are in 
				//			any of the aabbs, or inlining the is_inside directly into the ifs that add the primitives
				b8 in_bbleft, in_bfleft, in_tbleft, in_tfleft, in_bbright, in_bfright, in_tbright, in_tfright;

				in_bbleft = is_inside(current_node->primitives[i].face_vertices, bb_left.aabb);
				in_bfleft = is_inside(current_node->primitives[i].face_vertices, bf_left.aabb);
				in_tbleft = is_inside(current_node->primitives[i].face_vertices, tb_left.aabb);
				in_tfleft = is_inside(current_node->primitives[i].face_vertices, tf_left.aabb);
				in_bbright = is_inside(current_node->primitives[i].face_vertices, bb_right.aabb);
				in_bfright = is_inside(current_node->primitives[i].face_vertices, bf_right.aabb);
				in_tbright = is_inside(current_node->primitives[i].face_vertices, tb_right.aabb);
				in_tfright = is_inside(current_node->primitives[i].face_vertices, tf_right.aabb);

				if (in_bbleft)	
				{
					prim_bbleft.add(current_node->primitives[i]);
				}
				if (in_bfleft) 
				{
					prim_bfleft.add(current_node->primitives[i]);
				}
				if (in_tbleft)	
				{
					prim_tbleft.add(current_node->primitives[i]);
				}
				if (in_tfleft) 
				{
					prim_tfleft.add(current_node->primitives[i]);
				}
				if (in_bbright)	
				{
					prim_bbright.add(current_node->primitives[i]);
				}
				if (in_bfright) 
				{
					prim_bfright.add(current_node->primitives[i]);
				}
				if (in_tbright)	
				{
					prim_tbright.add(current_node->primitives[i]);
				}
				if (in_tfright) 
				{
					prim_tfright.add(current_node->primitives[i]);
				}
			}

			bb_left.primitives.size = prim_bbleft.length;
			bb_left.primitives.front = prim_bbleft.front;

			bf_left.primitives.size = prim_bfleft.length;
			bf_left.primitives.front = prim_bfleft.front;
			
			tb_left.primitives.size = prim_tbleft.length;
			tb_left.primitives.front = prim_tbleft.front;
			
			tf_left.primitives.size = prim_tfleft.length;
			tf_left.primitives.front = prim_tfleft.front;
			
			bb_right.primitives.size = prim_bbright.length;
			bb_right.primitives.front = prim_bbright.front;

			bf_right.primitives.size = prim_bfright.length;
			bf_right.primitives.front = prim_bfright.front;

			tb_right.primitives.size = prim_tbright.length;
			tb_right.primitives.front = prim_tbright.front;

			tf_right.primitives.size = prim_tfright.length;
			tf_right.primitives.front = prim_tfright.front;

			ASSERT(bb_left.primitives.size + bf_left.primitives.size + tb_left.primitives.size + 
				tf_left.primitives.size + bb_right.primitives.size + bf_right.primitives.size + tb_right.primitives.size +
				tf_right.primitives.size  >= current_node->primitives.size);
			current_node->primitives.clear();

			current_node->children_start_position = tree->tree.length;

			//adding nodes to the tree
			tree->tree.add_nocpy(bb_left);
			tree->tree.add_nocpy(bf_left);
			tree->tree.add_nocpy(tb_left);
			tree->tree.add_nocpy(tf_left);

			tree->tree.add_nocpy(bb_right);
			tree->tree.add_nocpy(bf_right);
			tree->tree.add_nocpy(tb_right);
			tree->tree.add_nocpy(tf_right);

			//adding nodes to be processed in node_stack
			node_stack.add(current_node->children_start_position);
			node_stack.add(current_node->children_start_position + 1);
			node_stack.add(current_node->children_start_position + 2);
			node_stack.add(current_node->children_start_position + 3);
			node_stack.add(current_node->children_start_position + 4);
			node_stack.add(current_node->children_start_position + 5);
			node_stack.add(current_node->children_start_position + 6);
			node_stack.add(current_node->children_start_position + 7);
		}
		else
		{
			current_node->has_children = FALSE;
		}
	}
	node_stack.clear_buffer();
}

struct TraversalData
{
	Optimized_Ray* ray;
	KD_Tree* tree;
	TriangleIntersectionData* tri_data;
	f32* closest;
};
//defined in renderer.cpp


static void traverse_oct_tree_new(TraversalData& td, KD_Tree& tree, KD_Node** hit_stack_front, LeafNodePair* leaf_stack_front);

f32 get_ray_kd_tree_intersection(Optimized_Ray& op_ray, KD_Tree& tree, TriangleIntersectionData& tri_data, KD_Node** hit_stack_front, LeafNodePair* leaf_stack_front)
{
	f32 closest = MAX_FLOAT;

	TraversalData td;
	td.closest = &closest;
	td.ray = &op_ray;
	td.tri_data = &tri_data;
	td.tree = &tree;
	
	traverse_oct_tree_new(td, tree, hit_stack_front, leaf_stack_front);
	return closest;
	
	//switch (tree.max_divisions)
	//{
	//case KD_Divisions::TWO:
	//{
	//	traverse_binary_tree(td, tree.tree.front);
	//	return closest;
	//}break;
	//case KD_Divisions::FOUR:
	//{
	//	traverse_quad_tree(td, tree.tree.front);
	//	return closest;
	//}break;
	//case KD_Divisions::EIGHT:
	//{
	//	//traverse_oct_tree_recursive(td, tree.tree.front);
	//	return closest;
	//}break;
	//default:
	//	ASSERT(FALSE);	//max_divisions traversal isn't defined
	//}
}

static void traverse_oct_tree_new(TraversalData& td, KD_Tree& tree, KD_Node** hit_stack_front, LeafNodePair* leaf_stack_front)
{
	if (!check_ray_AABB_intersection(*td.ray, tree.tree.front->aabb))
	{
		return;
	}

	if (tree.tree.front->has_children == 0)	//if there is no nodes other than root
	{
		KD_Primitive* prim = tree.tree.front->primitives.front;
		for (uint32 i = 0; i < tree.tree.front->primitives.size; i++)
		{
			f32 u, v;
			f32 distance = get_triangle_ray_intersection_culled(td.ray->ray, prim->face_vertices, u, v);
			if (distance < *td.closest && distance > tolerance)
			{
				*td.closest = distance;
				td.tri_data->face_index = tree.tree.front->primitives[i].face_index;
				td.tri_data->u = u;
				td.tri_data->v = v;
			}
			prim++;
		}
		return;
	}

	int32 leaf_stack_length = 0;
	*hit_stack_front = tree.tree.front;
	int32 hit_stack_length = 1;


	while (hit_stack_length > 0)
	{
		KD_Node* cur = *(hit_stack_front + hit_stack_length - 1);
		hit_stack_length--;
		KD_Node* children = tree.tree.front + cur->children_start_position;
		int nodes_hit = 0;	//ASSESS: whether this is actually a significant optimization or not. A ray cant hit more than 4 child nodes.
		for (int i = 0; i < 8 && nodes_hit <= 4; i++)
		{
			if (children->has_children)
			{
				if (check_ray_AABB_intersection(*td.ray, children->aabb))
				{
					nodes_hit++;
					*(hit_stack_front + hit_stack_length) = children;
					hit_stack_length++;
				}
			}
			else
			{
				f32 dis = get_ray_AABB_intersection(*td.ray, children->aabb);
				if (dis > 0.0f)
				{
					nodes_hit++;
					//inserting into leaf_stack in ascending manner
					if (leaf_stack_length == 0)
					{
						*leaf_stack_front = { children,dis };
						leaf_stack_length++;
					}
					else
					{

						//NOTE:if new element is less than first in list (lower than all elements), 
						//the barrier element before leaf_stack.front will stop the while and will add the new element at the beginning.
						LeafNodePair* end = leaf_stack_front + leaf_stack_length - 1;
						while (dis < end->distance_from_ray)
						{
							*(end + 1) = *end;
							end--;
						}
						*(end + 1) = { children,dis };
						leaf_stack_length++;

					}

				}
				/*if (check_ray_AABB_intersection(*td.ray, children->aabb))
				{
					nodes_hit++;
					KD_Primitive* prim = children->primitives.front;
					for (uint32 i = 0; i < children->primitives.size; i++)
					{
						f32 u, v;
						f32 distance = get_triangle_ray_intersection_culled(td.ray->ray, prim->face_vertices, u, v);
						if (distance < *td.closest && distance > tolerance)
						{
							*td.closest = distance;
							td.tri_data->face_index = children->primitives[i].face_index;
							td.tri_data->u = u;
							td.tri_data->v = v;
						}
						prim++;
					}
				}*/
			}
			children++;
		}
	}

	//start checking for primitive interesections from beginning of sorted list of leaf_stack
	LeafNodePair* cur = leaf_stack_front;
	b32 hit = FALSE;
	for (int j = 0; j < leaf_stack_length; j++)
	{
		KD_Primitive* prim = cur->node->primitives.front;
		for (uint32 i = 0; i < cur->node->primitives.size; i++)
		{
			f32 u, v;
			f32 distance = get_triangle_ray_intersection_culled(td.ray->ray, prim->face_vertices, u, v);
			if (distance < *td.closest && distance > tolerance)
			{
				*td.closest = distance;
				td.tri_data->face_index = cur->node->primitives[i].face_index;
				td.tri_data->u = u;
				td.tri_data->v = v;
				hit = TRUE;
			}
			prim++;
		}
		if (hit)
		{
			break;
		}
		cur++;
	}


}
static void traverse_binary_tree(TraversalData& td, KD_Node* current_node)
{
	if (!get_ray_AABB_intersection(*td.ray, current_node->aabb))
	{
		return;
	}
	if (current_node->has_children)
	{
		traverse_binary_tree(td, &td.tree->tree[current_node->children_start_position]);	//left node
		traverse_binary_tree(td, &td.tree->tree[current_node->children_start_position + 1]);	//right node
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

static void traverse_quad_tree(TraversalData& td, KD_Node* current_node)
{
	if (!get_ray_AABB_intersection(*td.ray, current_node->aabb))
	{
		return;
	}
	if (current_node->has_children)
	{
		traverse_quad_tree(td, &td.tree->tree[current_node->children_start_position]);	//bleft node
		traverse_quad_tree(td, &td.tree->tree[current_node->children_start_position + 1]);	//bright node
		traverse_quad_tree(td, &td.tree->tree[current_node->children_start_position + 2]);	//tleft node
		traverse_quad_tree(td, &td.tree->tree[current_node->children_start_position + 3]);	//tright node
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


static void traverse_oct_tree_recursive(TraversalData& td, KD_Node* current_node)
{
	if (!get_ray_AABB_intersection(*td.ray, current_node->aabb))
	{
		return;
	}
	if (current_node->has_children)
	{
		traverse_oct_tree_recursive(td, &td.tree->tree[current_node->children_start_position]);	
		traverse_oct_tree_recursive(td, &td.tree->tree[current_node->children_start_position + 1]);	
		traverse_oct_tree_recursive(td, &td.tree->tree[current_node->children_start_position + 2]);	
		traverse_oct_tree_recursive(td, &td.tree->tree[current_node->children_start_position + 3]);	
		traverse_oct_tree_recursive(td, &td.tree->tree[current_node->children_start_position + 4]);	
		traverse_oct_tree_recursive(td, &td.tree->tree[current_node->children_start_position + 5]);	
		traverse_oct_tree_recursive(td, &td.tree->tree[current_node->children_start_position + 6]);	
		traverse_oct_tree_recursive(td, &td.tree->tree[current_node->children_start_position + 7]);	
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
				b8 left_is_inside = is_inside(current_node->primitives[i].face_vertices, left.aabb);
				b8 right_is_inside = is_inside(current_node->primitives[i].face_vertices, right.aabb);

				if (left_is_inside)	//a vertex is in left aabb
				{
					primitives_left.add(current_node->primitives[i]);
				}
				if (right_is_inside) //a vertex is in right aabb
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

static void build_quad_kd_tree(KD_Tree* tree)
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

			KD_Node b_left = {}, b_right = {}, t_left = {}, t_right = {};

			AABB& p = current_node->aabb;
			vec3f& v = division_point;
			//cuts along Z-Y plane.

			b_left.aabb.min = p.min;
			b_left.aabb.max = { v.x, v.y, p.max.z };

			b_right.aabb.min = { v.x,p.min.y, p.min.z };
			b_right.aabb.max = { p.max.x, v.y, p.max.z };

			t_left.aabb.min = { p.min.x,v.y, p.min.z };
			t_left.aabb.max = { v.x,p.max.y, p.max.z };

			t_right.aabb.min = { v.x, v.y, p.min.z };
			t_right.aabb.max = p.max;



			//filling subnode primitives
			DBuffer<KD_Primitive, 0, 0, uint32> prim_tleft, prim_bleft, prim_tright, prim_bright;
			uint32 temp_buffer_cap_and_addon = (current_node->primitives.size / 4);	//using quarter of the parent nodes primitive size as a cap and overflow addon value
			prim_tleft.capacity = temp_buffer_cap_and_addon;
			prim_tleft.overflow_addon = temp_buffer_cap_and_addon;

			prim_bleft.capacity = temp_buffer_cap_and_addon;
			prim_bleft.overflow_addon = temp_buffer_cap_and_addon;

			prim_tright.capacity = temp_buffer_cap_and_addon;
			prim_tright.overflow_addon = temp_buffer_cap_and_addon;

			prim_bright.capacity = temp_buffer_cap_and_addon;
			prim_bright.overflow_addon = temp_buffer_cap_and_addon;


			for (uint32 i = 0; i < current_node->primitives.size; i++)
			{
				//testing if triangle is in the left or right or both
				b8 in_bleft, in_bright, in_tleft, in_tright;

				in_bleft = is_inside(current_node->primitives[i].face_vertices, b_left.aabb);
				in_bright = is_inside(current_node->primitives[i].face_vertices, b_right.aabb);
				in_tleft = is_inside(current_node->primitives[i].face_vertices, t_left.aabb);
				in_tright = is_inside(current_node->primitives[i].face_vertices, t_right.aabb);

				if (in_bleft)	//a vertex is in left aabb
				{
					prim_bleft.add(current_node->primitives[i]);
				}
				if (in_bright) //a vertex is in right aabb
				{
					prim_bright.add(current_node->primitives[i]);
				}
				if (in_tleft)	//a vertex is in left aabb
				{
					prim_tleft.add(current_node->primitives[i]);
				}
				if (in_tright) //a vertex is in right aabb
				{
					prim_tright.add(current_node->primitives[i]);
				}
			}

			b_left.primitives.front = prim_bleft.front;
			b_left.primitives.size = prim_bleft.length;

			b_right.primitives.front = prim_bright.front;
			b_right.primitives.size = prim_bright.length;

			t_left.primitives.front = prim_tleft.front;
			t_left.primitives.size = prim_tleft.length;

			t_right.primitives.front = prim_tright.front;
			t_right.primitives.size = prim_tright.length;

			ASSERT(b_left.primitives.size + b_right.primitives.size + t_left.primitives.size + t_right.primitives.size >= current_node->primitives.size);
			current_node->primitives.clear();

			current_node->children_start_position = tree->tree.length;

			//adding left and right to the tree
			tree->tree.add_nocpy(b_left);
			tree->tree.add_nocpy(b_right);
			tree->tree.add_nocpy(t_left);
			tree->tree.add_nocpy(t_right);
			//adding left and right to be processed in node_stack
			node_stack.add(current_node->children_start_position);
			node_stack.add(current_node->children_start_position + 1);
			node_stack.add(current_node->children_start_position + 2);
			node_stack.add(current_node->children_start_position + 3);
		}
		else
		{
			current_node->has_children = FALSE;
		}
	}
}
