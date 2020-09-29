#include "app.h"
#include "utilities/texture.h"

int main()
{
	Texture texture;

	Setup_Texture(texture, TextureFileType::BMP,1920, 1080);

	render_app(texture);

	Write_To_File(texture, "Results\\resultings");
}