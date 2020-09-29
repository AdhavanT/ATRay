#include "tile_work_queue.h"
#include <corecrt_malloc.h>

void create_tile_work_queue(TileWorkQueue& twq, Texture& tex)
{
	int32 tile_width = tex.bmb.width / get_core_count();	//ASSESS: which tile_width value gives best results
	int32 tile_height = tile_width;	// for square tiles

	ASSERT(tile_width > 0 && tile_height > 0 && tile_height <= (int32)tex.bmb.height);

	int32 no_x_tiles = (tex.bmb.width + tile_width - 1) / tile_width;
	int32 no_y_tiles = (tex.bmb.height + tile_height - 1) / tile_height;

	int32 total_tiles = no_y_tiles * no_x_tiles;

	twq.no_of_tiles = total_tiles;
	twq.tiles = (Tile*)malloc(sizeof(Tile) * total_tiles);
	twq.current_tile = 0;


	Tile* tmp = twq.tiles;

	//Creates the "tiles" and adds it into a "tile_work_queue"
	for (int y = 0; y < no_y_tiles; y++)
	{
		for (int x = 0; x < no_x_tiles; x++)
		{
			uint32 minx, miny, maxx, maxy;
			minx = x * tile_width;
			miny = y * tile_height;
			maxx = minx + tile_width;
			maxy = miny + tile_height;
			if (maxx > tex.bmb.width - 1)
			{
				maxx = (tex.bmb.width - 1);
			}
			if (maxy > tex.bmb.height - 1)
			{
				maxy = (tex.bmb.height - 1);
			}
			tmp->left_top = { (int32)minx, (int32)miny };
			tmp->right_bottom = { (int32)maxx, (int32)maxy };
			tmp = tmp++;
		}
	}
}

void free_tile_work_queue(TileWorkQueue& twq)
{
	if (twq.tiles)
	{
		free(twq.tiles);
	}
}