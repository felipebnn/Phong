#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

struct BoundingBox {
	glm::vec3 min;
	glm::vec3 max;
};
