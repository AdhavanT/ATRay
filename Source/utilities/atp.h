#pragma once
#define ATP_USE_CONFIG
/*
TODO for anyone: This Library doesn't use any C++ features (mostly just syntax) and can easily be ported to C.
          
                 For more info -> github.com/AdhavanT/ATProfiler

Consider adding the "atp_config.atp" file to easily configure and set custom macros 

ATP is a simple-to-use profiler built for games and other real-time applications.
ATP focuses on low performance overhead and being easy to use.
The trade-off is building the various "TestType" buffers at program start-up .
 Example:

        REGISTER_TEST(timer_name);    //This must be in global scope

        void do_stuff()
        {
            ATP_START(timer_name);
            ..do stuff i guess...
            ATP_END(timer_name);
        }

 Example using scoped object :

        REGISTER_TEST(timer_name);    //This must be in global scope

        void do_stuff()
        {
            ATP_BLOCK(timer_name);   //Creates a simple object whose destructor calls ATP_END() at scope end.
            ..do stuff i guess...    
        }

*/
#ifdef ATP_USE_CONFIG
#include "atp_config.atp"
#endif

#ifdef ATP_ONLY_ON_DEBUG
    #ifndef _DEBUG
        #define ATP_TURN_OFF
    #endif
#endif

#ifndef ATP_TURN_OFF


//TODO: Dont use this
#ifdef _DEBUG
    #define ATP_ASSERT(x) if(!(x)) __debugbreak();
#else
    #define ATP_ASSERT(X)
#endif 

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

#endif

namespace ATP
{

    typedef int  int32;
    typedef long long int64;
    typedef unsigned long long uint64;
    typedef float f32;
    typedef double f64;

    struct TestInfo
    {
        uint64 test_run_cycles;
    };

    struct TestType
    {
        const char* name;
        TestType* next_node;
        uint64 hits;
        TestInfo info;
    };


    extern TestType* testtype_list_global_front;
    extern TestType* testtype_list_global_last;

}
namespace ATP
{
    TestType register_testtype(TestType* new_testtype, const char* name_);
    //TODO: implement proper data structure to store all TestTypes (maybe hash map) 
    //that allows easy flushing of memory if not ATP_RECORD_ALL
    TestType* lookup_testtype(const char* name);

    //TODO: Agian, do this properly
    f64 get_ms_from_test(TestType &test);
}

#ifndef ATP_TURN_OFF

namespace ATP
{
#ifdef ATP_USE_QPC

    struct ATP_SCOPED_TIMER_BLOCK
    {
        TestType* testtype;
        LARGE_INTEGER cycles;
        ATP_SCOPED_TIMER_BLOCK(TestType* tt)
            :testtype(tt)
        {
            QueryPerformanceCounter(&cycles);
        }
        ~ATP_SCOPED_TIMER_BLOCK()
        {
            LARGE_INTEGER tmp;
            QueryPerformanceCounter(&tmp);
            testtype->info.test_run_cycles = tmp.QuadPart - cycles.QuadPart;
            testtype->hits++;
        }
    };
#else 
    struct ATP_SCOPED_TIMER_BLOCK
    {
        TestType* testtype;
        uint64 cycles;
        ATP_SCOPED_TIMER_BLOCK(TestType* tt)
            :testtype(tt)
        {
            cycles = __rdtsc();
        }
        ~ATP_SCOPED_TIMER_BLOCK()
        {
            testtype->info.test_run_cycles = __rdtsc() - cycles;
            testtype->hits++;
        }
    };
#endif
}

    #define ATP_REGISTER(name) \
     ATP::TestType name##ATPTEST = ATP::register_testtype(&name##ATPTEST,#name)


    #ifdef ATP_USE_QPC
        #define ATP_START(name) \
        LARGE_INTEGER ATP_CYCLES_##name;\
        QueryPerformanceCounter(&ATP_CYCLES_##name)

        #define ATP_END(name) \
        LARGE_INTEGER ATP_CYCLES_TMP##name;\
        QueryPerformanceCounter(&ATP_CYCLES_TMP##name);\
        name##ATPTEST.info.test_run_cycles = ATP_CYCLES_TMP##name.QuadPart - ATP_CYCLES_##name.QuadPart;\
        name##ATPTEST.hits++

    #else
        #define ATP_START(name) \
        uint64 ATP_CYCLES_##name = __rdtsc()

        #define ATP_END(name) \
        ATP_CYCLES_##name = __rdtsc() - ATP_CYCLES_##name;\
        name##ATPTEST.info.test_run_cycles = ATP_CYCLES_##name;\
        name##ATPTEST.hits++
    
    #endif

        #define ATP_BLOCK(name) \
        ATP::ATP_SCOPED_TIMER_BLOCK ATP_SCOPED_TIMER_##name(&name##ATPTEST)
#else

    #define ATP_REGISTER(name)
    #define ATP_START(name)
    #define ATP_END(name)
    #define ATP_BLOCK(name)
#endif