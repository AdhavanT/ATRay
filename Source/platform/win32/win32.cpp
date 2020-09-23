#include "app.h"
#include "custom headers/bitmap.h"
#include "custom headers/atp.h"
#include "platform/platform.h"
#include <windows.h>


uint32 get_core_count()
{
	static uint32 core_count = 0;
	if (core_count == 0)
	{
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);
		core_count = sys_info.dwNumberOfProcessors;
	}
	return 8;
}

int64 interlocked_add(volatile int64* data, int64 value)
{
	return InterlockedAdd64(data, value);
}

void close_threads(uint32 no_of_threads, const ThreadHandle* handles)
{
	for (uint32 i = 0; i < no_of_threads; i++)
	{
		CloseHandle(handles[i].thread_handle);
	}
}

void wait_for_all_threads(uint32 no_of_threads, const ThreadHandle* handles, uint32 timeout_in_ms)
{
	WaitForMultipleObjects(no_of_threads, (HANDLE*)handles, TRUE, timeout_in_ms);
}


struct CreateThreadData
{
	ThreadProc func_to_be_executed;
	void* data;
};

static DWORD WINAPI win32_start_thread(__in LPVOID lpParameter)
{
	CreateThreadData * new_thread_data = (CreateThreadData*)lpParameter;
	new_thread_data->func_to_be_executed(new_thread_data->data);
	free(new_thread_data);
	return 0;
}

ThreadHandle create_thread(ThreadProc proc, void* data)
{
	CreateThreadData *new_thread_data = (CreateThreadData*)malloc(sizeof(CreateThreadData));
	new_thread_data->func_to_be_executed = proc;
	new_thread_data->data = data;

	DWORD thread_id;
	HANDLE new_thread_handle = CreateThread(0, 0, win32_start_thread, (void*)new_thread_data, 0, &thread_id);
	ThreadHandle handle;
	handle.thread_handle = new_thread_handle;
	return handle;
}

int64 interlocked_increment(volatile int64* data)
{
	return InterlockedIncrement64(data);
}

int main()
{
	Bitmap bitmap;
	
	Setup_Bitmap(bitmap, 1280, 720);
	
	render_app(bitmap.bitmap_buffer);
	
	Create_BMP_File(bitmap, "Results\\resultings");
} 