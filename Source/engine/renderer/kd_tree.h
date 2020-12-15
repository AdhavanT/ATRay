#pragma once
#include "scene.h"

enum class KD_No_Of_Divisions
{
	TWO = 2,
	FOUR = 4,
	EIGHT = 8
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
		b32 is_leaf_node;	//is anything but zero if node has children
		int32 children_start_position;	//position in tree buffer where children are
	};
	FDBuffer<KD_Primitive> primitives;
};
struct KD_Tree
{
	//Possible number of subnodes for each node
	int32 max_no_faces_per_node;
	KD_No_Of_Divisions max_divisions;
	DBuffer<KD_Node,1,16,int32> tree;
};

static KD_Tree make_KD_tree(ModelData mdl,int32 _max_no_faces_per_node, KD_No_Of_Divisions _max_divisions)
{
	KD_Tree tree = {};
	tree.max_divisions = _max_divisions;
	tree.max_no_faces_per_node = _max_no_faces_per_node;
	KD_Node root = {};
	root.aabb = get_AABB(mdl);
	root.primitives.allocate(mdl.faces_vertices.size);
	KD_Primitive* prim = root.primitives.front;
	for (uint32 i = 0; i < root.primitives.size; i++)
	{
		TriangleVertices tri;
		tri.a = mdl.vertices[mdl.faces_vertices[i].vertex_indices[0]];
		tri.b = mdl.vertices[mdl.faces_vertices[i].vertex_indices[1]];
		tri.c = mdl.vertices[mdl.faces_vertices[i].vertex_indices[2]];

		*prim = { tri, i};

		prim++;
	}
	//ASSESS: whether I should clear the vertices buffer for mdl since it's copied to this tree.


}

//Handles adding, and setting aabb, for children nodes in tree
static void add_children_node_aabb_from_point(KD_Tree& tree, AABB parent_aabb,vec3f division_point)
{
	AABB& p = parent_aabb;
	vec3f& v = division_point;
	if (!is_inside(division_point, p))
	{
		ASSERT(FALSE);	//point is not inside p
	}
	switch (tree.max_divisions)
	{
		case KD_No_Of_Divisions::TWO:
		{
			//cuts along Z-Y plane.
			AABB left = {}, right = {};
			left.min = p.min;
			left.max.x = v.x;
			left.max.y = p.max.y;
			left.max.z = p.max.z;
			
			right.min.x = v.x;
			right.min.y = p.min.x;
			right.min.z = p.min.z;
			right.max = p.max;

			KD_Node node_left = {}, node_right = {};
			node_left.aabb = left;
			node_right.aabb = right;
			tree.tree.add_nocpy(node_left);
			tree.tree.add_nocpy(node_right);

		}break;
		case KD_No_Of_Divisions::FOUR:
		{
			//divides into 4 quadrants. Cuts along Z-Y plane and X-Z plane
			AABB b_left = {}, b_right = {}, t_left = {}, t_right = {};
			b_left.min = p.min;
			b_left.max = { v.x, v.y, p.max.z };

			b_right.min = { v.x,p.min.y, p.min.z };
			b_right.max = { p.max.x, v.y, p.max.z };

			t_left.min = { p.min.x,v.y, p.min.z };
			t_left.max = { v.x,p.max.y, p.max.z };
			
			t_right.min = { v.x, v.y, p.min.z };
			t_right.max = p.max;

			KD_Node nb_left = {}, nb_right = {}, nt_left = {}, nt_right = {};
			nb_left.aabb = b_left;
			nb_right.aabb = b_right;
			nt_left.aabb = t_left;
			nt_right.aabb = t_right;
			tree.tree.add_nocpy(nb_left);
			tree.tree.add_nocpy(nb_right);
			tree.tree.add_nocpy(nt_left);
			tree.tree.add_nocpy(nt_right);

		}break;
		case KD_No_Of_Divisions::EIGHT:
		{
			//TODO:do this
		}break;
		default:
			ASSERT(FALSE);	//improper max divisions setting for tree
	}
}