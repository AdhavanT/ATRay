#pragma once
#include "model.h"

enum KD_No_Of_Divisions
{
	TWO = 2,
	FOUR = 4,
	EIGHT = 8
};

struct KD_Node
{
	AABB aabb;
	union
	{
		b32 is_leaf_node;
		int32 children_start_position;
	};
	
};
struct KD_Tree
{
	//Possible number of subnodes
	int32 min_no_faces_per_node;
	KD_No_Of_Divisions max_divisions;
	DBuffer<KD_Node> tree;
};