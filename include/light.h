#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

struct Light {
	glm::vec3 pos;
	glm::vec3 color;
};
