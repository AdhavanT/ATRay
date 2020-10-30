#include "engine/app.h"
#include "utilities/texture.h"

int main()
{
	Texture texture;

	Setup_Texture(texture, TextureFileType::BMP,1920, 1080);

	ThreadPool thread_pool;
	//thread_pool.threads.allocate(1);	//For single thread
	thread_pool.threads.allocate(get_core_count());

	render_app(texture,thread_pool);

	Write_To_File(texture, "Results\\resultings");
}