#pragma once

#include <corecrt_malloc.h>	//for malloc and free
#include <stdio.h>	//for printf()

//This is used as a guide for platform specific code that needs to be implemented 
//and added at the end.

#ifndef RELEASE
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

//------------------------------------------<THREADING>--------------------------------------------
//Data for a handle to a thread
struct ThreadHandle
{
	void* thread_handle;
};

//What a thread callback function looks like
typedef void (*ThreadProc)(void*);

//creates thread and returns a handle to the thread (only use functions defined in this header to operate on handle)
ThreadHandle create_thread(ThreadProc proc, void * data);

//release thread handles
void close_threads(uint32 no_of_threads, const ThreadHandle* handles);

//waits for all threads to be released
void wait_for_all_threads(uint32 no_of_threads, const ThreadHandle* handles, uint32 timeout_in_ms);

//gets unique thread id
uint32 get_thread_id();

//returns resulting incremented value after performing locked increment
int64 interlocked_increment(volatile int64*);

//returns resulting decremented value after performing locked decrement
int64 interlocked_decrement(volatile int64* data);

//performs atomic add and returns result
int64 interlocked_add(volatile int64*, int64);

//gets number of cpu cores
uint32 get_core_count();

//gets a random uint64 number from system 
//NOTE: this entropy may be biased based on how it is implemented
uint64 get_hardware_entropy();
//------------------------------------------</THREADING>--------------------------------------------

//--------------------------------------<FILE I/O>------------------------------------
//Opens a file of location "path" into *file. type can be "r" to open for reading, "rb" reading for binary,(standard C file io types) etc...
b32 file_open(void** file, const char* path, const char* type);

//Closes a file handle. Returns true if successful. 
b32 file_close(void* file);

//Will try to load the file into a memory block. Returns number of bytes read.
//(check if returned and bytes_to_read are equal to check for success) 
uint32 load_from_file(void* file, uint32 bytes_to_read, void* block_to_store);

uint32 get_file_size(void* file);

//--------------------------------------</FILE I/O>------------------------------------

