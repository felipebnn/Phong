#include "kdnode.h"

#include <algorithm>

std::unique_ptr<KdNode> KdNode::buildKdNode(Triangle* trianglesX, size_t triangleCount, int depth) {
	std::unique_ptr<KdNode> node = std::make_unique<KdNode>();

	if (triangleCount == 1) {
		node->triangle = *trianglesX;
		node->bbox = BoundingBox::fromTriangle(*trianglesX);
	} else if (triangleCount > 1) {
		Triangle* mid = trianglesX + triangleCount / 2;

		std::nth_element(trianglesX, mid, trianglesX + triangleCount, [depth] (const Triangle& t0, const Triangle& t1) {
			return t0.v0->pos[depth] + t0.v1->pos[depth] + t0.v2->pos[depth] < t1.v0->pos[depth] + t1.v1->pos[depth] + t1.v2->pos[depth];
		});

		depth = (depth + 1) % 3;

		node->left = buildKdNode(trianglesX, static_cast<size_t>(mid - trianglesX), depth);
		node->right = buildKdNode(mid, static_cast<size_t>(trianglesX + triangleCount - mid), depth);

		node->bbox = node->left->bbox;
		node->bbox.expand(node->right->bbox);
	}

	return node;
}
