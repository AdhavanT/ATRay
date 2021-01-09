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
	//uint32 pad[6];
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
	//uint32 pad[7];
};
struct KD_Tree
{
	//max no of triangles per node
	uint32 max_no_faces_per_node;
	//Possible number of subnodes for each node
	KD_Divisions max_divisions;
	//How to decide the subnode division point when building tree
	KD_Division_Method division_method;
	DBuffer<KD_Node,1,16,int32> tree;
};

struct LeafNodePair
{
	KD_Node* node;
	f32 distance_from_ray;
};
void build_KD_tree(ModelData mdl, KD_Tree& tree);

f32 get_ray_kd_tree_intersection(Optimized_Ray& op_ray, KD_Tree& tree, TriangleIntersectionData& tri_data, KD_Node** hit_stack_front, LeafNodePair* leaf_stack_front);
