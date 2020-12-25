#pragma once
#include "ray.h"

//This KD tree supports a single model.

enum class KD_Divisions
{
	TWO = 2,
	FOUR = 4,
	EIGHT = 8
};

enum class KD_Division_Method
{
	CENTER, SAH
};

struct KD_Primitive
{
	//TODO: try padding this to 64 bytes to fit a cache line. 
	TriangleVertices face_vertices;
	uint32 face_index;
	
};

struct KD_Node
{
	//TODO: try padding this to 64 bytes to fit a cache line. 
	AABB aabb;
	union
	{
		b32 has_children;	//same as !is_leaf_node
		int32 children_start_position;	//position in tree buffer where children are
	};
	FDBuffer<KD_Primitive,uint32> primitives;
};
struct KD_Tree
{
	//max no of triangles per node
	int32 max_no_faces_per_node;
	//Possible number of subnodes for each node
	KD_Divisions max_divisions;
	//How to decide the subnode division point when building tree
	KD_Division_Method division_method;
	DBuffer<KD_Node,1,16,int32> tree;
};

void create_binary_kd_tree(KD_Tree* tree, KD_Node* top_node);

void build_KD_tree(ModelData mdl, KD_Tree& tree);

//intersection test between ray and 2 subnode tree
f32 get_ray_kd_tree_intersection(Optimized_Ray& ray, KD_Tree& tree, TriangleIntersectionData& tri_data);

//
//static void create_subnodes(KD_Tree& tree, KD_Node& current_node)
//{
//	if (current_node.primitives.size > tree.max_no_faces_per_node)
//	{
//		vec3f sah_point = (current_node.aabb.max - current_node.aabb.min) / 2; //TODO: find SAH point
//		current_node.children_start_position = tree.tree.length;
//		add_children_node_aabb_from_point(tree, current_node.aabb, sah_point);
//		//fill subnode primitives
//		//free current_node primitives
//		//foreach subnode, recursively call same function
//	}
//	else
//	{
//		current_node.is_leaf_node = TRUE;
//	}

//}

//Handles adding, and setting aabb, for children nodes in tree
//static void add_children_node_aabb_from_point(KD_Tree& tree, AABB parent_aabb,vec3f division_point)
//{
//	AABB& p = parent_aabb;
//	vec3f& v = division_point;
//	if (!is_inside(division_point, p))
//	{
//		ASSERT(FALSE);	//point is not inside p
//	}
//	switch (tree.max_divisions)
//	{
//		case KD_No_Of_Divisions::TWO:
//		{
//			//cuts along Z-Y plane.
//			AABB left = {}, right = {};
//			left.min = p.min;
//			left.max.x = v.x;
//			left.max.y = p.max.y;
//			left.max.z = p.max.z;
//			
//			right.min.x = v.x;
//			right.min.y = p.min.x;
//			right.min.z = p.min.z;
//			right.max = p.max;
//
//			KD_Node node_left = {}, node_right = {};
//			node_left.aabb = left;
//			node_right.aabb = right;
//			tree.tree.add_nocpy(node_left);
//			tree.tree.add_nocpy(node_right);
//
//		}break;
//		case KD_No_Of_Divisions::FOUR:
//		{
//			//divides into 4 quadrants. Cuts along Z-Y plane and X-Z plane
//			AABB b_left = {}, b_right = {}, t_left = {}, t_right = {};
//			b_left.min = p.min;
//			b_left.max = { v.x, v.y, p.max.z };
//
//			b_right.min = { v.x,p.min.y, p.min.z };
//			b_right.max = { p.max.x, v.y, p.max.z };
//
//			t_left.min = { p.min.x,v.y, p.min.z };
//			t_left.max = { v.x,p.max.y, p.max.z };
//			
//			t_right.min = { v.x, v.y, p.min.z };
//			t_right.max = p.max;
//
//			KD_Node nb_left = {}, nb_right = {}, nt_left = {}, nt_right = {};
//			nb_left.aabb = b_left;
//			nb_right.aabb = b_right;
//			nt_left.aabb = t_left;
//			nt_right.aabb = t_right;
//			tree.tree.add_nocpy(nb_left);
//			tree.tree.add_nocpy(nb_right);
//			tree.tree.add_nocpy(nt_left);
//			tree.tree.add_nocpy(nt_right);
//
//		}break;
//		case KD_No_Of_Divisions::EIGHT:
//		{
//			//TODO:do this
//		}break;
//		default:
//			ASSERT(FALSE);	//improper max divisions setting for tree
//	}
//}