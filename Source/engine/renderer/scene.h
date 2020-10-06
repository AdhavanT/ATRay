#pragma once
#include "sphere.h"
#include "plane.h"
#include "model.h"

struct Scene
{
	//TODO: These are DBuffers for now. They will be FDBuffers when scene loading is implemented
	//and the sizes are known before adding
	DBuffer<Material> materials;

	DBuffer<Model> models;

	DBuffer<Sphere> spheres;

	DBuffer<Plane> planes;
};

