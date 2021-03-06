#pragma once

#include "triangle.h"

#include <limits>

struct HitInfo {
	float t = std::numeric_limits<float>::max();
	float u, v;
	Triangle const* triangle;

	inline operator bool() {
		return t < std::numeric_limits<float>::max();
	}
};
