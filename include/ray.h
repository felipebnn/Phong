#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "triangle.h"
#include "boundingbox.h"
#include "kdnode.h"
#include "hitinfo.h"

struct Ray {
	glm::vec3 orig;
	glm::vec3 dir;

	bool intersectTriangle(const Triangle& triangle, HitInfo& hitInfo) const;
	bool intersectKdNode(KdNode* node, HitInfo& hitInfo) const;
	bool intersectBoundingBox(const BoundingBox& bbox) const;
};
