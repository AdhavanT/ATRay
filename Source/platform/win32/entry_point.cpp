#include "app.h"
#include "custom headers/bitmap.h"

int main()
{
	Bitmap bitmap;

	Setup_Bitmap(bitmap, 1280, 720);

	render_app(bitmap.bitmap_buffer);
	//benchmark();

	Create_BMP_File(bitmap, "Results\\resultings");
}