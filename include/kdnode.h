#pragma once

#include <vector>
#include <memory>

#include "boundingbox.h"
#include "triangle.h"

struct KdNode {
	BoundingBox bbox;
	std::unique_ptr<KdNode> left;
	std::unique_ptr<KdNode> right;
	Triangle triangle;

	static std::unique_ptr<KdNode> buildKdNode(Triangle* trianglesX, size_t triangleCount, int depth);
};
