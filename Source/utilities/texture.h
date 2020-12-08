#pragma once

#include "PL/PL_math.h"

#ifdef _DEBUG
#define ASSERT(x) if(!(x)) __debugbreak();
#else
#define ASSERT(X)
#endif 

enum struct TextureFileType
{
	BMP
};

struct BitmapBuffer
{
	void* buffer_memory;
	uint32 bytes_per_pixel;
	uint32 width;
	uint32 height;

	inline uint32 size() { return bytes_per_pixel * width * height; };
};
struct Texture
{
	TextureFileType file_type;
	BitmapBuffer bmb;
};


//takes RGB color and sets it to [x,y] point on bitmap
inline void Set_Pixel(const Vec3<uint8>& color, Texture& texture, int32 x, int32 y)
{
	if (texture.file_type == TextureFileType::BMP)
	{
		Vec3<uint8> rgb_color = { color.z, color.y,color.x };
		ASSERT(((((uint32)x <= texture.bmb.width) && (x >= 0)) && (((uint32)y <= texture.bmb.height) && (y >= 0))));

		uint32* pixel = (uint32*)texture.bmb.buffer_memory;
		pixel += y * texture.bmb.width + x;
		*pixel = *(uint32*)&rgb_color.raw[0] & 0x00FFFFFF;
	}
}

bool Setup_Texture(Texture& tex, TextureFileType file_type, uint32 width, uint32 height);

bool Write_To_File(Texture& texture, const char* file_name);