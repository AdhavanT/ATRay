#pragma once

#ifdef _DEBUG
#define ASSERT(x) if(!(x)) __debugbreak();
#else
#define ASSERT(X)
#endif 

//-----------------------------------------------
typedef signed char        int8;
typedef short              int16;
typedef int                int32;
typedef long long          int64;
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

typedef bool b8;
typedef int b32;

typedef float f32;
typedef double f64;
//-----------------------------------------------


//-----------platform specific------------------
//These structs need to be redefined or reassessed on platform change 
//as their definition contains platform specific objects

struct ThreadHandle
{
	void* thread_handle;
};

//----------------------------------------------


typedef void (*ThreadProc)(void*);

//creates thread and returns a handle to the thread (only use functions defined in this header to operate on handle)
ThreadHandle create_thread(ThreadProc proc, void * data);

//release thread handles
void close_threads(uint32 no_of_threads, const ThreadHandle* handles);

void wait_for_all_threads(uint32 no_of_threads, const ThreadHandle* handles, uint32 timeout_in_ms);

//returns resulting incremented value after performing locked increment
int64 interlocked_increment(volatile int64*);

//performs atomic add and returns result
int64 interlocked_add(volatile int64*, int64);

//gets number of cpu cores
uint32 get_core_count();