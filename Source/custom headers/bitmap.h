#pragma once
#define _CRT_SECURE_NO_DEPRECATE 
#include <cstdio>
#include <string>

//Making sure to avoid packing. Size should be 14 bytes.
#pragma pack(push,bfh, 2)
struct BitmapFileHeader
{
	uint8   bitmap_type[2];
	int32   total_file_size;
	int16   reserved1;
	int16   reserved2;
	uint32	offset_bits;
};
#pragma pack(pop,bfh)

//Size should be 56 bytes
struct BitmapDIBHeader
{
	uint32    size_header;
	uint32    width;
	uint32    height;
	int16     planes;
	int16     bits_per_pixel;
	uint32    compression;
	uint32    buffer_size;
	uint32    ppm_x;
	uint32    ppm_y;
	uint32    clr_used;
	uint32    clr_important;
	//DO NOT MESS WITH MASK VALUES
	uint32    red_bitmask;
	uint32    green_bitmask;
	uint32    blue_bitmask;
	uint32    alpha_bitmask;
};

struct BitmapBuffer
{
	void* buffer_memory;
	uint32 bytes_per_pixel;
	uint32 width;
	uint32 height;

	inline uint32 size() { return bytes_per_pixel * width * height; };
};

struct Bitmap
{
	BitmapFileHeader bfh = {};
	BitmapDIBHeader bih = {};
	BitmapBuffer bitmap_buffer = {};

};

static void Setup_Bitmap(Bitmap& bmp, uint32 width, uint32 height)
{
	bmp.bitmap_buffer.bytes_per_pixel = 4;
	bmp.bitmap_buffer.height = height;
	bmp.bitmap_buffer.width = width;
	bmp.bitmap_buffer.buffer_memory = malloc(bmp.bitmap_buffer.size());

	bmp.bih.bits_per_pixel = 8 * bmp.bitmap_buffer.bytes_per_pixel;
	bmp.bih.planes = 1;
	bmp.bih.height = bmp.bitmap_buffer.height;
	bmp.bih.width = bmp.bitmap_buffer.width;
	bmp.bih.clr_important = 0;
	bmp.bih.clr_used = 0;
	bmp.bih.compression = 3;
	bmp.bih.buffer_size = bmp.bitmap_buffer.size();
	bmp.bih.size_header = sizeof(bmp.bih);
	bmp.bih.ppm_x = 197;
	bmp.bih.ppm_y = 39;
	bmp.bih.red_bitmask = 0x00FF0000;
	bmp.bih.green_bitmask = 0x0000FF00;
	bmp.bih.blue_bitmask = 0x000000FF;
	bmp.bih.alpha_bitmask = 0x00000000;

	bmp.bfh.bitmap_type[0] = 'B';
	bmp.bfh.bitmap_type[1] = 'M';
	bmp.bfh.offset_bits = 14 + sizeof(bmp.bih);
	bmp.bfh.reserved1 = 0;
	bmp.bfh.reserved2 = 0;
	bmp.bfh.total_file_size = 14 + sizeof(bmp.bih) + bmp.bitmap_buffer.size();

}

static bool Create_BMP_File(Bitmap& bmp, const char* file_name)
{
	FILE* file;
	
	uint32 file_id = 0;
	int length = strlen(file_name);
	
	char* new_name = (char*)malloc(length + 8);

	snprintf(new_name, length + 8, "%s%c%u%s", file_name, '_', file_id,".bmp");

	//Checking if another file exists with same name
	while (fopen_s(&file, new_name, "r") != ENOENT)
	{
		fclose(file);
		file_id++;
		snprintf(new_name, length + 8, "%s%c%u%s", file_name, '_', file_id,".bmp");

	}
	if (file) 
	{ 
		fclose(file); 
	}
	
	if (fopen_s(&file, new_name, "wb"))
	{
		free(new_name);
		return false;
	}

	fwrite(&bmp.bfh, 1, 14, file);
	fwrite(&bmp.bih, sizeof(bmp.bih), 1, file);
	fwrite(bmp.bitmap_buffer.buffer_memory, bmp.bitmap_buffer.size(), 1, file);
	fclose(file);
	free(new_name);
	return true;
}

//takes RGB color and sets it to [x,y] point on bitmap
inline void Set_Pixel(const int32 x, const int32 y, const Vec3<uint8>& color, BitmapBuffer& buffer)
{
	Vec3<uint8> rgb_color = { color.z, color.y,color.x };
	ASSERT(((((uint32)x <= buffer.width) && (x >= 0)) && (((uint32)y <= buffer.height) && (y >= 0))));

	uint32* pixel = (uint32*)buffer.buffer_memory;
	pixel += y * buffer.width + x;
	*pixel = *(uint32*)&rgb_color.raw[0] & 0x00FFFFFF;

}
