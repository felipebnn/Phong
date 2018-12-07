#include "kdnode.h"

#include <algorithm>

KdNode::KdNode(std::vector<Triangle>::iterator begin, std::vector<Triangle>::iterator end, int depth) {
	ptrdiff_t len = end - begin;

	if (len == 1) {
		triangle = *begin;
		bbox = BoundingBox::fromTriangle(*begin);
	} else if (len > 1) {
		auto mid = begin + len / 2;

		std::nth_element(begin, mid, end, [depth] (const Triangle& t0, const Triangle& t1) {
			return t0.v0->pos[depth] + t0.v1->pos[depth] + t0.v2->pos[depth] < t1.v0->pos[depth] + t1.v1->pos[depth] + t1.v2->pos[depth];
		});

		depth = (depth + 1) % 3;

		left = std::make_unique<KdNode>(begin, mid, depth);
		right = std::make_unique<KdNode>(mid, end, depth);

		bbox = left->bbox;
		bbox.expand(right->bbox);
	}
}
