#include "atp.h"


#ifndef ATP_TURN_OFF

namespace ATP
{

    TestType* testtype_list_global_front = nullptr;
    TestType* testtype_list_global_last = nullptr;

    TestType register_testtype(TestType* new_testtype,const char* name_)
    {
        new_testtype->name = name_;
        if (testtype_list_global_front == nullptr)
        {
            testtype_list_global_front = new_testtype;
            testtype_list_global_last = new_testtype;
            new_testtype->next_node = nullptr;
        }
        else
        {
            testtype_list_global_last->next_node = new_testtype;
            testtype_list_global_last = new_testtype;
            new_testtype->next_node = nullptr;
        }
        return *new_testtype;
    }

    
    TestType* lookup_testtype(const char* name)
    {
        TestType *tmp = testtype_list_global_front;
        while (name != tmp->name)
        {
            tmp = tmp->next_node;
        }
        if (tmp == nullptr)
        {
            ATP_ASSERT(false);
            return nullptr;
        }
        else
        {
            return tmp;
        }
    }
    f64 get_ms_from_test(TestType& test)
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        f64 time_elapsed = (test.info.test_run_cycles * 1000  / (f64)frequency.QuadPart) ;
        return time_elapsed;
    }
}
#endif

#ifdef ATP_TURN_OFF
namespace ATP
{
    TestType* testtype_list_global_front = nullptr;
    TestType* testtype_list_global_last = nullptr;
    
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