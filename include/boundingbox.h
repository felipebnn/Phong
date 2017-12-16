#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "triangle.h"

struct BoundingBox {
	glm::vec3 min;
	glm::vec3 max;

	inline BoundingBox& expand(const BoundingBox& otherBbox) {
		min = glm::min(min, otherBbox.min);
		max = glm::max(max, otherBbox.max);

		return *this;
	}

	inline BoundingBox& expand(const Triangle& triangle) {
		return expand(fromTriangle(triangle));
	}

	inline static BoundingBox fromTriangle(const Triangle& triangle) {
		return {
			glm::min(glm::min(triangle.v0->pos, triangle.v1->pos), triangle.v2->pos),
			glm::max(glm::max(triangle.v0->pos, triangle.v1->pos), triangle.v2->pos)
		};
	}
};
