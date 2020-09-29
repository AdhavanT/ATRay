#pragma once
#include "sphere.h"
#include "plane.h"
#include "model.h"

struct Scene
{
	Material skybox;

	int32 no_of_models;
	Model* models;

	int32 no_of_spheres;
	Sphere* spheres;

	int32 no_of_planes;
	Plane* planes;

};