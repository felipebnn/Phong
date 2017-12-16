#include "boundingbox.h"

void BoundingBox::expand(const BoundingBox& otherBbox) {
	min = glm::min(min, otherBbox.min);
	max = glm::max(max, otherBbox.max);
}

void BoundingBox::expand(const Triangle& triangle) {
	expand(fromTriangle(triangle));
}
