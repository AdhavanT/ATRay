#include "app.h"
#include "utilities/atp.h"
#include "utilities/fileio.h"


ATP_REGISTER(render_app);

ATP_REGISTER(prep_scene);
ATP_REGISTER(render_from_camera);

//ASSESS: How I wanna store a "Face" and whether I want to parse the model like this and do pre-processing later
struct Model_Face
{
	int32 vertices[3];
	int32 vertex_normals[3];
	int32 texture_coords[3];
};

struct OBJ_Model_Load_Data
{
	DBuffer<vec3f,50,100> vertices;
	DBuffer<vec3f,50,100> normals;
	DBuffer<Model_Face,20,50> faces;
};

struct ParseDataChunk
{
	int32 chunk_size;
	char* start;
	char* end;

	OBJ_Model_Load_Data chunk_data;
};

//TODO: Load and parse the Deer.obj file. 
void load_model(Model& mdl, const char* file_name)
{
	void* file = nullptr;
	b32 opened = file_open(&file,file_name, "rb");

	ASSERT(opened);	//Can't load file.
	
	uint32 file_size = get_file_size(file);

	int32 no_of_chunks = get_core_count();

	char* buffer_front = (char*)malloc(file_size);

	uint8 file_load = load_from_file(file, file_size, buffer_front);

	int32 general_chunk_size = file_size + (no_of_chunks - 1) / no_of_chunks;
	FDBuffer<ParseDataChunk>chunks;
	ParseDataChunk* chunk = chunks.init(no_of_chunks);

	char* ptr = buffer_front;
	char* file_end = (buffer_front + (file_size - 1));
	while (ptr < file_end)
	{
		uint32 cursor = 0;
		chunk->start = ptr;
		cursor += general_chunk_size;
		if ((ptr + cursor) > file_end)
		{
			cursor = file_end - ptr;
		}
		else
		{
			while (*(ptr + cursor) != '\n')
			{
				cursor++;
			}
		}
		ptr += cursor;
		chunk->chunk_size = cursor;
		chunk->end = ptr;
		ptr++;
	}

	b32 closed = file_close(file);
	ASSERT(closed);	//Can't close file
}

void print_out_tests();

void render_app(Texture& texture)
{
	
	Model mdl_loaded;
	load_model(mdl_loaded,"Assets\\Deer.obj");

	ATP_START(render_app);

	Camera cm;
	RenderSettings rs;
	rs.no_of_threads = get_core_count();
	rs.anti_aliasing = true;
	rs.resolution.x = texture.bmb.width;
	rs.resolution.y = texture.bmb.height;
	rs.samples_per_pixel = 64;
	rs.bounce_limit = 5;

	set_camera(cm, { 0.f,1.f,0.f }, { 0.f, 0.0f,-1.f }, rs, 1.0f);

	Scene scene;
	Material skybox;


	Sphere spr[2];
	Plane pln[2];
	Model mdl[1];
	TriangleVertices tri[1];

	tri[0].a = { -3.0f, 3.0f, -5.0f };
	tri[0].b = { -1.0f, 3.0f, -5.0f };
	tri[0].c = { -2.0f, 1.0f, -5.0f };
	mdl[0].triangles.add(tri[0]);

	spr[0].center = { 0.f,1.0f,-7.f };
	spr[0].radius = 1.0f;
	spr[0].material.color = { 0.f,0.8f,0.9f };
	spr[0].material.specularity = 0.3f;

	spr[1].center = { 2.f,1.5f,-7.f };
	spr[1].radius = 1.f;
	spr[1].material.color = { 0.9f,0.4f,0.2f };
	spr[1].material.specularity = 0.9f;


	pln[0].material.color = { 0.2f, 0.3f,0.2f };
	pln[0].material.specularity = 0.f;
	pln[0].distance = 7.f;
	pln[0].normal = { -1.f,0.f,0.f };

	//ground plane
	pln[1].material.color = { 0.9f, 0.8f,0.2f };
	pln[1].material.specularity= 0.f;
	pln[1].distance = 0.f;
	pln[1].normal = { 0.f,1.f,0.f };

	scene.models.add(mdl[0]);
	scene.planes.add(pln[0]);
	scene.planes.add(pln[1]);
	scene.spheres.add(spr[0]);
	scene.spheres.add(spr[1]);

	scene.skybox.specularity = 1.0f;
	scene.skybox.color = { 0.1f,0.1f,0.1f };

	
	ATP_START(prep_scene);
	prep_scene(scene);
	ATP_END(prep_scene);

	printf("\nResolution [%i,%i] || Samples per pixel - %i - Starting Render...\n",texture.bmb.width, texture.bmb.height, rs.samples_per_pixel);
	
	ATP_START(render_from_camera);
	int64 rays_shot = render_from_camera(cm, scene, texture);
	ATP_END(render_from_camera);

	ATP_END(render_app);

	printf("\nCompleted:\n");
	
	print_out_tests();
	
	printf("	Total Rays Shot: %I64i rays\n", rays_shot);
	printf("	Millisecond Per Ray: %.*f ms/ray\n", 8, ATP::get_ms_from_test(*ATP::lookup_testtype("render_from_camera")) / (f64)rays_shot);

}

void print_out_tests()
{
	int32 length;
	ATP::TestType* front = ATP::get_testtype_registry(length);
	for (int i = 0; i < length; i++)
	{
		printf("	Time Elapsed(ATP->%s):%.*f ms\n",front->name,3, ATP::get_ms_from_test(*front));
		front++;
	}
}