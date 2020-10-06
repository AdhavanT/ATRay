#pragma once

#include "renderer/model.h"
#include "thread_pool.h"

void load_model(ModelData& mdl, const char* file_name, ThreadPool& threadpool);

