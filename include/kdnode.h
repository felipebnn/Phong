#pragma once

#include <memory>

#include "boundingbox.h"
#include "triangle.h"

struct KdNode {
	BoundingBox bbox;
	std::unique_ptr<KdNode> left;
	std::unique_ptr<KdNode> right;
	Triangle* triangles;
	size_t triangleCount;
};
