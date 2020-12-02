#pragma once

#include <stdio.h>	//for printf()

//This is used as a guide for platform specific code that needs to be implemented 
//and added at the end.

#ifndef RELEASE
#define ASSERT(x) if(!(x)) __debugbreak();
#else
#define ASSERT(X)
#endif 

//-----------------------------------------------
#define MAX_FLOAT          3.402823466e+38F        // max value
#define MIN_FLOAT          1.175494351e-38F        // min normalized positive value
#define UINT32MAX		   0xffffffff			
#define INV_UINT32_MAX	   2.328306437e-10F

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
//-------------------------------------------<MEMORY ALLOCATION>-------------------------------------------
//Copies a buffer of size "length" into destination
void buffer_copy(void* destination, void* from, uint32 length);
void* buffer_malloc(size_t size);
void* buffer_calloc(size_t size);
void* buffer_realloc(void* block, size_t new_size);
void buffer_free(void* buffer);
extern int32 freed, allocations;
//-------------------------------------------</MEMORY ALLOCATION>-------------------------------------------

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

//release thread handle
void close_thread(const ThreadHandle* handle);

//release thread handles
void close_threads(uint32 no_of_threads, const ThreadHandle* handles);

//Waits for thread to finish or timeout
void wait_for_thread(const ThreadHandle* handle, uint32 timeout_in_ms);

//waits for all threads to be released
void wait_for_all_threads(uint32 no_of_threads, const ThreadHandle* handles, uint32 timeout_in_ms);

//gets unique thread id
uint32 get_thread_id();

//performs atomic add and returns result(for int64 value)
int64 interlocked_add_i64(volatile int64*, int64);
//performs atomic add and returns result(for int32 value)
int32 interloacked_add_i32(volatile int32* data, int32 value);


//returns resulting incremented value after performing locked increment(for int64 value)
int64 interlocked_increment_i64(volatile int64*);
//returns resulting decremented value after performing locked decrement(for int32 value)
int64 interlocked_increment_i32(volatile int32* data);


//returns resulting decremented value after performing locked decrement(for int64 value)
int64 interlocked_decrement_i64(volatile int64* data);
//returns resulting decremented value after performing locked decrement(for int32 value)
int64 interlocked_decrement_i32(volatile int32* data);


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

//returns file size
uint32 get_file_size(void* file);

//--------------------------------------</FILE I/O>------------------------------------

//--------------------------------------<SYSTEM WIDE DEBUG DEFINES>-------------------
//#define TURN_OFF_RANDOM
//--------------------------------------</SYSTEM WIDE DEBUG DEFINES>-------------------
