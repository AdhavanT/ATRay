#pragma once
#include "sphere.h"
#include "plane.h"
#include "model.h"

struct Scene
{
	Material skybox;

	DBuffer<Model> models;

	DBuffer<Sphere> spheres;

	DBuffer<Plane> planes;
};

void init_scene(Scene& scene);