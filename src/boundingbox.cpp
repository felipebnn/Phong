#include "boundingbox.h"

BoundingBox& BoundingBox::expand(const BoundingBox& otherBbox) {
	min = glm::min(min, otherBbox.min);
	max = glm::max(max, otherBbox.max);

	return *this;
}

BoundingBox& BoundingBox::expand(const Triangle& triangle) {
	return expand(fromTriangle(triangle));
}
