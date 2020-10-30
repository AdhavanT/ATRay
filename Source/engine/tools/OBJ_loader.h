#pragma once

#include "engine/renderer/model.h"
#include "thread_pool.h"

void load_model_data(ModelData& mdl, const char* file_name, ThreadPool& threadpool);

