#pragma once
#include "utilities/types.h"
#include "utilities/texture.h"
#include "engine/tools/work_queue.h"
#include "scene.h"
#include "camera.h"


int64 render_from_camera(PL& pl,Camera& cm, Scene& scene, Texture& tex, ThreadPool& tpool);

void prep_scene(Scene);
