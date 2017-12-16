#include "kdnode.h"

std::unique_ptr<KdNode> KdNode::buildKdNode(Triangle* triangles, size_t triangleCount, int depth) {
	std::unique_ptr<KdNode> node = std::make_unique<KdNode>();
	node->triangles = triangles;
	node->triangleCount = triangleCount;

	if (triangleCount == 1) {
		node->bbox = BoundingBox::fromTriangle(triangles[0]);
	}

	if (triangleCount < 2) {
		return node;
	}

	std::sort(triangles, triangles + triangleCount, [depth] (const Triangle& t0, const Triangle& t1) {
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

	Triangle* mid = triangles + triangleCount / 2;

	node->left = buildKdNode(triangles, mid - triangles, depth + 1);
	node->right = buildKdNode(mid, triangles + triangleCount - mid, depth + 1);

	node->bbox = node->left->bbox;
	node->bbox.expand(node->right->bbox);

	return node;
}
