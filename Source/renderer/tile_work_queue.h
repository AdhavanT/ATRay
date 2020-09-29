#pragma once
#include "utilities/types.h"
#include "utilities/texture.h"

struct TileWorkQueue
{
	int64 no_of_tiles;
	Tile* tiles;
	volatile int64 current_tile = 0;
};

void create_tile_work_queue(TileWorkQueue& twq, Texture& tex);
void free_tile_work_queue(TileWorkQueue& twq);