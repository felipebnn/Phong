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

	KdNode(std::vector<Triangle>::iterator begin, std::vector<Triangle>::iterator end, int depth=0);
};
