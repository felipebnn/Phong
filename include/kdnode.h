#pragma once

#include <memory>

#include "boundingbox.h"

struct KdNode {
	BoundingBox bbox;
	std::unique_ptr<KdNode> left;
	std::unique_ptr<KdNode> right;
	size_t* triangles;
	size_t triangleCount;
};
