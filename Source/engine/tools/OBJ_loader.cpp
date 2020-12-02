#include "OBJ_loader.h"
#include "utilities/fileio.h"
#include "work_queue.h"

//Possible Optimizations:
//Perfrom prep_faces in SIMD ( easy to remove the offset on multiple data)
//Perform parse_chunks in SIMD 



struct OBJ_Model_Load_Chunk_Data
{
	DBuffer<vec3f, 50, 100, uint32> vertices;
	DBuffer<vec3f, 50, 100, uint32> normals;
	DBuffer<vec3f, 50, 100, uint32> tex_coords;
	DBuffer<FaceVertices, 20, 50, uint32> faces_vertices;
	DBuffer<FaceData, 20, 50, uint32> faces_data;

};

struct ParseDataChunk
{
	int32 chunk_size = 0;
	char* start;
	char* end;

	OBJ_Model_Load_Chunk_Data chunk_data;
};



static bool parse_chunks(WorkQueue<ParseDataChunk>& chunks_)
{
	ParseDataChunk* chunk;
	int64 chunk_no = interlocked_increment_i32(&chunks_.jobs_done);
	if (chunk_no > chunks_.jobs.size)
	{
		interlocked_decrement_i32(&chunks_.jobs_done);
		return false;
	}
	else
	{
		chunk = &chunks_.jobs[(int32)chunk_no - 1];
	}

	char* cursor = chunk->start;
	vec3f parsed_vec3f;
	while (cursor < chunk->end)
	{
		switch (*cursor)
		{
		case 'v':

		{	cursor++;
			switch (*cursor)
			{
				case ' ':
				{	cursor = parse_vec3f(cursor, parsed_vec3f);
					chunk->chunk_data.vertices.add(parsed_vec3f);
					break;
				}
				case 't':
				{	cursor++;
					cursor = parse_vec3f(cursor, parsed_vec3f);
					chunk->chunk_data.tex_coords.add(parsed_vec3f);
					break;
				}
				case 'n':
				{	cursor++;
					cursor = parse_vec3f(cursor, parsed_vec3f);
					chunk->chunk_data.normals.add(parsed_vec3f);
					break;
				}
			}
			break;
		}
		case 'f':

		{	
		cursor++;

		FaceVertices tmp_v;
		FaceData tmp_d;

		cursor = parse_int(cursor, tmp_v.vertex_indices[0]);				//getting v0
		if (*cursor == '/')
		{
			cursor++;
			if (*cursor == '/')
			{
				cursor++;
				cursor = parse_int(cursor, tmp_d.vertex_normals_indices[0]);  //To parse v0//vn0 format
			}
			else
			{
				cursor = parse_int(cursor, tmp_d.tex_coord_indices[0]);	//To parse v0/vt0 format
				if (*cursor == '/')
				{
					cursor++;
					cursor = parse_int(cursor, tmp_d.vertex_normals_indices[0]);//To parse v0/vt0/vn0 format
				}
			}
		}

		cursor = parse_int(cursor, tmp_v.vertex_indices[1]);
		if (*cursor == '/')
		{
			cursor++;
			if (*cursor == '/')
			{
				cursor++;
				cursor = parse_int(cursor, tmp_d.vertex_normals_indices[1]);
			}
			else
			{
				cursor = parse_int(cursor, tmp_d.tex_coord_indices[1]);
				if (*cursor == '/')
				{
					cursor++;
					cursor = parse_int(cursor, tmp_d.vertex_normals_indices[1]);
				}
			}
		}
		cursor = parse_int(cursor, tmp_v.vertex_indices[2]);
		if (*cursor == '/')
		{
			cursor++;
			if (*cursor == '/')
			{
				cursor++;
				cursor = parse_int(cursor, tmp_d.vertex_normals_indices[2]);
			}
			else
			{
				cursor = parse_int(cursor, tmp_d.tex_coord_indices[2]);
				if (*cursor == '/')
				{
					cursor++;
					cursor = parse_int(cursor, tmp_d.vertex_normals_indices[2]);
				}
			}
		}
		chunk->chunk_data.faces_data.add_nocpy(tmp_d);
		chunk->chunk_data.faces_vertices.add_nocpy(tmp_v);

		break;
		}

		case 'u':
		{	if (*(cursor + 1) == 's' &&
			*(cursor + 2) == 'e' &&
			*(cursor + 3) == 'm' &&
			*(cursor + 4) == 't' &&
			*(cursor + 5) == 'l' &&
			*(cursor + 6) == ' ')
			{
				cursor = cursor + 6;
				//TODO: Get Material Name to use
			}
			break;
		}
		case '#':
			break;

		case '\n':
			break;

		default:
			break;
		}
		cursor = skip_to_new_line(cursor);
		cursor++;
	}
	
	return true;
}


static void start_parse_chunk_thread(void* chunks_)
{
	WorkQueue<ParseDataChunk>* chunks = (WorkQueue<ParseDataChunk>*)chunks_;

	while (parse_chunks(*chunks))
	{
		debug_print("\rChunks parsed: %i/%i", chunks->jobs_done, chunks->jobs.size);
	}
}

//Joins the parsed chunks into a big chunk. NOTE: Joining needs to be in order of parsing to keep indicies integrity intact.
static void join_chunks(WorkQueue<ParseDataChunk>& chunks_, ModelData& chunk)
{
	//ASSESS: Should I multi-thread this or is that overkill? Nevermind....probably overkill..
	uint32 no_faces = 0;
	uint32 no_vertices = 0;
	uint32 no_normals = 0;
	uint32 no_tex_coords = 0;

	for (int i = 0; i < chunks_.jobs.size; i++)
	{
		no_faces += chunks_.jobs[i].chunk_data.faces_vertices.length;
		no_normals += chunks_.jobs[i].chunk_data.normals.length;
		no_vertices += chunks_.jobs[i].chunk_data.vertices.length;
		no_tex_coords += chunks_.jobs[i].chunk_data.tex_coords.length;
	}
	//Allocating the final chunk of memory that holds all the chunks.
	vec3f* ptr_tex_chunk = chunk.tex_coords.allocate(no_tex_coords);
	vec3f* ptr_nor_chunk = chunk.normals.allocate(no_normals);
	vec3f* ptr_vert_chunk = chunk.vertices.allocate(no_vertices);
	FaceVertices* ptr_face_chunk = chunk.faces_vertices.allocate(no_faces);
	FaceData* ptr_face_data_chunk = chunk.faces_data.allocate(no_faces);


	for (int i = 0; i < chunks_.jobs.size; i++)
	{
		buffer_copy(ptr_tex_chunk, chunks_.jobs[i].chunk_data.tex_coords.front, chunks_.jobs[i].chunk_data.tex_coords.length * sizeof(vec3f));
		buffer_copy(ptr_nor_chunk, chunks_.jobs[i].chunk_data.normals.front, chunks_.jobs[i].chunk_data.normals.length * sizeof(vec3f));
		buffer_copy(ptr_vert_chunk, chunks_.jobs[i].chunk_data.vertices.front, chunks_.jobs[i].chunk_data.vertices.length * sizeof(vec3f));
		buffer_copy(ptr_face_chunk, chunks_.jobs[i].chunk_data.faces_vertices.front, chunks_.jobs[i].chunk_data.faces_vertices.length * sizeof(FaceVertices));
		buffer_copy(ptr_face_data_chunk, chunks_.jobs[i].chunk_data.faces_data.front, chunks_.jobs[i].chunk_data.faces_data.length * sizeof(FaceData));

		ptr_face_chunk += chunks_.jobs[i].chunk_data.faces_vertices.length;
		ptr_face_data_chunk += chunks_.jobs[i].chunk_data.faces_data.length;
		ptr_nor_chunk += chunks_.jobs[i].chunk_data.normals.length;
		ptr_vert_chunk += chunks_.jobs[i].chunk_data.vertices.length;
		ptr_tex_chunk += chunks_.jobs[i].chunk_data.tex_coords.length;
	}
}

static void prep_model_data(ModelData& mdl)
{
	//Making all the negative indicies proper. (negative indicies are relative to the end of the list)
	for (uint32 i = 0; i < mdl.faces_data.size; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (mdl.faces_data[i].tex_coord_indices[j] < 0)
			{
				mdl.faces_data[i].tex_coord_indices[j] = mdl.tex_coords.size + mdl.faces_data[i].tex_coord_indices[j] + 1;
			}
			if (mdl.faces_data[i].vertex_normals_indices[j] < 0)
			{
				mdl.faces_data[i].vertex_normals_indices[j] = mdl.normals.size + mdl.faces_data[i].vertex_normals_indices[j] + 1;
			}
			if (mdl.faces_vertices[i].vertex_indices[j] < 0)
			{
				mdl.faces_vertices[i].vertex_indices[j] = mdl.vertices.size + mdl.faces_vertices[i].vertex_indices[j] + 1;
			}
		}
	}

	//Removing the +1 offset from indicies
	//NOTE: still makes missing Face data into -1. 
	for (uint32 i = 0; i < mdl.faces_data.size; i++)
	{
		mdl.faces_data[i].tex_coord_indices[0]--;
		mdl.faces_data[i].tex_coord_indices[1]--;
		mdl.faces_data[i].tex_coord_indices[2]--;

		mdl.faces_vertices[i].vertex_indices[0]--;
		mdl.faces_vertices[i].vertex_indices[1]--;
		mdl.faces_vertices[i].vertex_indices[2]--;

		mdl.faces_data[i].vertex_normals_indices[0]--;
		mdl.faces_data[i].vertex_normals_indices[1]--;
		mdl.faces_data[i].vertex_normals_indices[2]--;
	}
}

static inline void clear_OBJ_Model_Load_Chunk_Data(OBJ_Model_Load_Chunk_Data& data)
{
	data.faces_data.clear_buffer();
	data.faces_vertices.clear_buffer();
	data.normals.clear_buffer();
	data.tex_coords.clear_buffer();
	data.vertices.clear_buffer();
}

void load_model_data(ModelData& mdl, const char* file_name, ThreadPool& threadpool)
{

	uint32 file_size = get_file_size((char*)file_name);
	char* buffer_front = (char*)buffer_malloc(file_size + 1);
	load_file_into(buffer_front, file_size, (char*)file_name);

	if (buffer_front[file_size - 1] != 0)
	{
		buffer_front[file_size] = 0;
	}

	int32 no_of_chunks = threadpool.threads.size;
	


	int32 general_chunk_size = (file_size + (no_of_chunks - 1)) / no_of_chunks;
	WorkQueue<ParseDataChunk>chunks;
	ParseDataChunk* chunk = chunks.jobs.allocate_preserve_type_info(no_of_chunks);

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
		chunk++;
		ptr++;
	}
	chunks.jobs[chunks.jobs.size - 1].end++;		//accounting for off-by-one error ( now last chunk.end points to '\n')
	*chunks.jobs[chunks.jobs.size - 1].end = '\n';	//to make the last character a new line.

	activate_pool(threadpool, start_parse_chunk_thread, &chunks);
	wait_for_pool(threadpool);

	
	//joining the chunks together
	join_chunks(chunks, mdl);

	//preps the model data (removes index offset, etc..)
	prep_model_data(mdl);


	if (chunks.jobs_done == chunks.jobs.size)
	{
		//Freeing buffer thats filled with obj file 
		buffer_free(buffer_front);
		//Deleting chunks
		for (int i = 0; i < chunks.jobs.size; i++)
		{
			clear_OBJ_Model_Load_Chunk_Data(chunks.jobs[i].chunk_data);
		}
		//deleting job queue buffer
		chunks.jobs.clear();
	}
	else
	{
		ASSERT(false);	//All chunks were not parsed!
	}


}