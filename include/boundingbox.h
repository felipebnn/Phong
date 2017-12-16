#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "triangle.h"

struct Triangle;

struct BoundingBox {
	glm::vec3 min;
	glm::vec3 max;

	void expand(const BoundingBox& otherBbox);
	void expand(const Triangle& triangle);

	inline static BoundingBox fromTriangle(const Triangle& triangle) {
		return {
			glm::min(glm::min(triangle.v0->pos, triangle.v1->pos), triangle.v2->pos),
			glm::max(glm::max(triangle.v0->pos, triangle.v1->pos), triangle.v2->pos)
		};
	}
};
