#include "kdnode.h"

#include <algorithm>

std::unique_ptr<KdNode> KdNode::buildKdNode(Triangle* trianglesX, size_t triangleCount, int depth) {
	std::unique_ptr<KdNode> node = std::make_unique<KdNode>();

	if (triangleCount == 1) {
		node->triangle = *trianglesX;
		node->bbox = BoundingBox::fromTriangle(*trianglesX);
	}

	if (triangleCount < 2) {
		return node;
	}

	std::sort(trianglesX, trianglesX + triangleCount, [depth] (const Triangle& t0, const Triangle& t1) {
		glm::vec3 c0 = t0.v0->pos + t0.v1->pos + t0.v2->pos;
		glm::vec3 c1 = t1.v0->pos + t1.v1->pos + t1.v2->pos;

		switch (depth % 3) {
			case 0:
				return c0.x < c1.x;
			case 1:
				return c0.y < c1.y;
			default: //case 2:
				return c0.z < c1.z;
		}
	});

	Triangle* mid = trianglesX + triangleCount / 2;

	node->left = buildKdNode(trianglesX, mid - trianglesX, depth + 1);
	node->right = buildKdNode(mid, trianglesX + triangleCount - mid, depth + 1);

	node->bbox = node->left->bbox;
	node->bbox.expand(node->right->bbox);

	return node;
}
