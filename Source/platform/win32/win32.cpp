#include "app.h"
#include "custom headers/bitmap.h"
#include "custom headers/atp.h"
#include <windows.h>

int main()
{
	Bitmap bitmap;
	
	Setup_Bitmap(bitmap, 1920, 1080);
	
	LARGE_INTEGER start_count, end_count,difference, frequency;
	f64 time_elapsed_a;
	
	
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start_count);
	render_app(bitmap.bitmap_buffer);
	QueryPerformanceCounter(&end_count);
	


	difference.QuadPart = end_count.QuadPart - start_count.QuadPart;
	time_elapsed_a = (difference.QuadPart / (double)frequency.QuadPart);
	
	
	ATP::TestType *tt = ATP::lookup_testtype("render_app");
	f64 time_elapsed = ATP::get_ms_from_test(*tt);

	ATP::TestType* tc = ATP::lookup_testtype("cast_ray");
	f64 time_elapsed_cr = ATP::get_ms_from_test(*tc);

	printf("	Time Elapsed:actual: %.*f seconds\n", 3, time_elapsed_a );
	printf("	Time Elapsed:ATP: %.*f seconds\n", 3, time_elapsed / 1000);	  
	printf("	Time Elapsed:ATP: %.*f ms\n", 9, time_elapsed_cr);

	Create_BMP_File(bitmap, "Results\\resultings");
}