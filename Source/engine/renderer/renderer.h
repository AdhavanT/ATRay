#pragma once
#include "utilities/types.h"
#include "utilities/texture.h"
#include "utilities/atp.h"
#include "platform/platform.h"
#include "engine/work_queue.h"
#include "scene.h"
#include "camera.h"


int64 render_from_camera(Camera& cm, Scene& scene, Texture& tex, ThreadPool& tpool);

void prep_scene(Scene);
