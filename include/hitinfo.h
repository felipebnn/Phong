#pragma once

#include "triangle.h"

struct HitInfo {
	float t = std::numeric_limits<float>::max();
	float u, v;
	Triangle triangle;

	operator bool() {
		return t < std::numeric_limits<float>::max();
	}
};