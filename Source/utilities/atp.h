#ifndef ATP_HEADER_IS_INCLUDED
#define ATP_HEADER_IS_INCLUDED

#define ATP_USE_CONFIG

/*
TODO for anyone: This Library doesn't use any C++ features (mostly just syntax) and can easily be ported to C.
          
                 For more info -> github.com/AdhavanT/ATProfiler

Consider adding the "atp_config.atp" file to get the description of and configure macros 

ATP is a simple-to-use profiler built for games and other real-time applications.
ATP focuses on low performance overhead and being easy to use.
The trade-off is building the various "TestType" buffers at program start-up.
ATP builds a global buffer to hold all "TestType"s 
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
#else
//Defaults
#define ATP_TESTTYPE_BUFFER_INIT_CAPACITY 5
#define ATP_TESTTYPE_BUFFER_INIT_OVERFLOW_ADDON 5
#define ATP_USE_QPC
//#define ATP_ONLY_ON_DEBUG
//#define ATP_TURN_OFF

#endif


#ifdef ATP_ONLY_ON_DEBUG
    #ifndef _DEBUG
        #define ATP_TURN_OFF
    #endif
#endif


#ifdef _DEBUG
    #define ATP_ASSERT(x) if(!(x)) __debugbreak();
#else
    #define ATP_ASSERT(X)
#endif 

//Dependencies 

//Used for queryperformancecounter or rdtsc if on windows
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN     
#include <Windows.h>
#endif

//Used for memory allocation in free, malloc, realloc
#ifdef _WIN32 
#include <corecrt_malloc.h>
#else
#include <stdlib.h>
#endif
#define ATP_REALLOC realloc
#define ATP_CALLOC calloc
#define ATP_FREE free


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
        uint64 hits;
        TestInfo info;
    };

}
namespace ATP
{

    TestType* get_testtype_registry(int32& no_of_testtypes);

    //that allows easy flushing of memory if not ATP_RECORD_ALL
    TestType* lookup_testtype(const char* name);

    //TODO: Agian, do this properly
    f64 get_ms_from_test(TestType &test);
    
    TestType* register_testtype( const char* name_);
}

#ifndef ATP_TURN_OFF

namespace ATP
{
#ifdef _WIN32 

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
#endif
}

    #define ATP_REGISTER(name) \
     ATP::TestType *name##ATPTEST = ATP::register_testtype(#name)

//Used to call to a test registered in a different compilation unit 
    #define ATP_EXTERN_REGISTER(name) \
     extern ATP::TestType *name##ATPTEST;

    #ifdef ATP_USE_QPC
        #define ATP_START(name) \
        LARGE_INTEGER ATP_CYCLES_##name;\
        QueryPerformanceCounter(&ATP_CYCLES_##name)

        #define ATP_END(name) \
        LARGE_INTEGER ATP_CYCLES_TMP##name;\
        QueryPerformanceCounter(&ATP_CYCLES_TMP##name);\
        name##ATPTEST->info.test_run_cycles = ATP_CYCLES_TMP##name.QuadPart - ATP_CYCLES_##name.QuadPart;\
        name##ATPTEST->hits++

    #else
        #define ATP_START(name) \
        uint64 ATP_CYCLES_##name = __rdtsc()

        #define ATP_END(name) \
        ATP_CYCLES_##name = __rdtsc() - ATP_CYCLES_##name;\
        *name##ATPTEST.info.test_run_cycles = ATP_CYCLES_##name;\
        *name##ATPTEST.hits++
    
    #endif

        #define ATP_BLOCK(name) \
        ATP::ATP_SCOPED_TIMER_BLOCK ATP_SCOPED_TIMER_##name(name##ATPTEST)
#else
    #define ATP_REGISTER(name)              //Use to Register a TestType. Place in global scope.
    #define ATP_EXTERN_REGISTER(name)       //Use to Call a TestType registered in a different compilation unit
    #define ATP_START(name)                 //Use to start timer 
    #define ATP_END(name)                   //Use to end timer and register timer info
    #define ATP_BLOCK(name)                 //Scoped ATP_START() and ATP_END() block
#endif


#ifndef ATP_IS_IMPLEMENTATION

//Removing libraries macros 
#undef ATP_REALLOC 
#undef ATP_CALLOC 
#undef ATP_FREE 
#undef ATP_TESTTYPE_BUFFER_INIT_CAPACITY 
#undef ATP_TESTTYPE_BUFFER_INIT_OVERFLOW_ADDON 
#undef ATP_USE_QPC
#undef ATP_ONLY_ON_DEBUG
#undef ATP_TURN_OFF
#undef ATP_ASSERT 

#ifdef _WIN32
#undef WIN32_LEAN_AND_MEAN
#undef ATP_USE_QPC
#endif
#endif
#endif // !ATP_HEADER_IS_INCLUDED
