#include "ray.h"

bool Ray::intersectTriangle(const Triangle& triangle, HitInfo& hitInfo) const {
	glm::vec3 v0v1 = triangle.v1->pos - triangle.v0->pos;
	glm::vec3 v0v2 = triangle.v2->pos - triangle.v0->pos;
	glm::vec3 pvec = glm::cross(dir, v0v2);
	float det = glm::dot(v0v1, pvec);

	if (det > 0) return false; //culling

	float invDet = 1 / det;

	glm::vec3 tvec = orig - triangle.v0->pos;
	float u = glm::dot(tvec, pvec) * invDet;
	if (u < 0 || u > 1) return false;

	glm::vec3 qvec = glm::cross(tvec, v0v1);
	float v = glm::dot(dir, qvec) * invDet;
	if (v < 0 || u + v > 1.001f) return false;

	float t = glm::dot(v0v2, qvec) * invDet;

	if (t < hitInfo.t) {
		hitInfo.t = t;
		hitInfo.u = u;
		hitInfo.v = v;
		hitInfo.triangle = &triangle;
	}

	return true;
}

bool Ray::intersectKdNode(KdNode* node, HitInfo& hitInfo) const {
	if (intersectBoundingBox(node->bbox)) {
		if (node->left || node->right) {
			intersectKdNode(node->left.get(), hitInfo);
			intersectKdNode(node->right.get(), hitInfo);
		} else {
			intersectTriangle(node->triangle, hitInfo);
		}
	}

	return hitInfo;
}

bool Ray::intersectBoundingBox(const BoundingBox& bbox) const {
	float tmin = (bbox.min.x - orig.x) / dir.x;
	float tmax = (bbox.max.x - orig.x) / dir.x;

	if (tmin > tmax) {
		std::swap(tmin, tmax);
	}

	float tymin = (bbox.min.y - orig.y) / dir.y;
	float tymax = (bbox.max.y - orig.y) / dir.y;

	if (tymin > tymax) {
		std::swap(tymin, tymax);
	}

	if ((tmin > tymax) || (tymin > tmax)) {
		return false;
	}

	if (tymin > tmin) {
		tmin = tymin;
	}

	if (tymax < tmax) {
		tmax = tymax;
	}

	float tzmin = (bbox.min.z - orig.z) / dir.z;
	float tzmax = (bbox.max.z - orig.z) / dir.z;

	if (tzmin > tzmax) {
		std::swap(tzmin, tzmax);
	}

	if ((tmin > tzmax) || (tzmin > tmax)) {
		return false;
	}

	if (tzmin > tmin) {
		tmin = tzmin;
	}

	if (tzmax < tmax) {
		tmax = tzmax;
	}

	return true;
}
