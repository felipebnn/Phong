#pragma once

#include <algorithm>
#include <memory>

#include "boundingbox.h"
#include "triangle.h"

struct KdNode {
	BoundingBox bbox;
	std::unique_ptr<KdNode> left;
	std::unique_ptr<KdNode> right;
	Triangle* triangles;
	size_t triangleCount;

	static std::unique_ptr<KdNode> buildKdNode(Triangle* triangles, size_t triangleCount, int depth);
};
