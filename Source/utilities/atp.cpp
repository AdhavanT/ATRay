#define ATP_IS_IMPLEMENTATION
#include "atp.h"

//Used for memory allocation in free, malloc, realloc
#ifdef _WIN32 
#include <corecrt_malloc.h>
#else
#include <stdlib.h>
#endif
#define ATP_REALLOC realloc
#define ATP_CALLOC calloc
#define ATP_FREE free


#ifndef ATP_TURN_OFF

namespace ATP
{

    struct TestType_DBuffer
    {
        int32 length;
        int32 capacity;
        TestType* front = (TestType*)0;

        TestType* add(TestType& new_member)
        {
            length++;
            if (length > capacity)
            {
                capacity = capacity + ATP_TESTTYPE_BUFFER_OVERFLOW_ADDON;
                TestType* temp = (TestType*)ATP_REALLOC(front, capacity * sizeof(TestType));
                ATP_ASSERT(temp);
                front = temp;
            }

            TestType* temp = front;
            temp = temp + (length - 1);
            *temp = new_member;
            return temp;
        }

        void clear_buffer()
        {
            length = 0;
            ATP_FREE(front);
        }

        inline TestType& at(int32 index)
        {
            ATP_ASSERT(index >= 0 && index < length);
            return (front[index]);
        }
    };

    //NOTE: This is a pointer that is dynamically allocated cause it'll be zeroed out on dynamic initillization
    //that happens before main and after all testtypes register themselves if it's a normal global variable
    TestType_DBuffer* global_testtypes;

    TestType* register_testtype(const char* name_)
    {
        if (global_testtypes == (TestType_DBuffer*)0)
        {
            global_testtypes = (TestType_DBuffer*)ATP_CALLOC(1, sizeof(TestType_DBuffer));      //creating the global_testtypes buffer to avoid zero-initilization from Ordered dynamic initialization
        }
        TestType test_type;
        test_type.hits = 0;
        test_type.info = { 0 };
        test_type.name = name_;
        if (global_testtypes->front == (TestType*)0) //If buffer is not initilized yet
        {
            global_testtypes->length = 0;
            global_testtypes->capacity = ATP_TESTTYPE_BUFFER_INIT_CAPACITY;
            global_testtypes->front = (TestType*)ATP_CALLOC(global_testtypes->capacity, sizeof(TestType));
        }
        return global_testtypes->add(test_type);
    }


    TestType* get_testtype_registry(int32& no_of_testtypes)
    {
        no_of_testtypes = global_testtypes->length;
        return global_testtypes->front;
    }

    TestType* lookup_testtype(const char* name)
    {
        for (int i = 0; i < global_testtypes->length; i++)
        {
            if (name == global_testtypes->at(i).name)
            {
                return &global_testtypes->at(i);
            }
        }
        return 0;
    }
    f64 get_ms_from_test(TestType& test)
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        f64 time_elapsed = (test.info.test_run_cycles * 1000 / (f64)frequency.QuadPart);
        return time_elapsed;
    }
}
#endif

#ifdef ATP_TURN_OFF
namespace ATP
{

    TestType* lookup_testtype(const char* name)
    {
        return nullptr;
    }

    f64 get_ms_from_test(TestType& test)
    {
        return 0;
    }
}

#endif
