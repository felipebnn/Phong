#pragma once

#define THREADED
#define BARYCENTER_INTERPOLATION

#include <csignal>
#include <cmath>

#include <iostream>
#include <fstream>
#include <vector>

#ifdef THREADED
#include <thread>
#include <mutex>
#endif

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image_write.h"
#include "tiny_obj_loader.h"

#include "vertex.h"
#include "light.h"
#include "kdnode.h"
#include "boundingbox.h"

class KdNode;

class Phong {
private:
	std::vector<Vertex> vertices;
	std::vector<Vertex> transformed_vertices;

	std::vector<Light> lights;
	std::vector<Light> transformed_lights;

	std::vector<uint32_t> imageData;

	int width;
	int height;
	size_t pixelCount;

	glm::mat4 model;
	glm::mat4 view;

	glm::vec3 albedo;
	float Kd;
	float Ks;
	float n;
	glm::vec3 ambient;

	glm::vec3 camera;

	constexpr static float epsilon = 1e-8;

	size_t drawingIndex;

	std::vector<size_t> triangles;
	std::unique_ptr<KdNode> kdTree;

	#ifdef THREADED
	std::mutex jobsMutex;
	std::vector<std::thread> workers;
	#endif

	void loadModel(const std::string& modelName);
	void loadScene(const std::string& sceneFileName);
	void applyTransformation();
	std::unique_ptr<KdNode> buildKdNode(size_t* triangles, size_t triangleCount, int depth);
	void buildKdTree();
	bool rayTriangleIntersect(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v);
	bool rayBoundingBoxIntersect(const glm::vec3& orig, const glm::vec3& dir, const BoundingBox& bbox);
	bool rayKdNodeIntersection(KdNode* node, const glm::vec3& orig, const glm::vec3& dir, float& t, glm::vec3& color);
	glm::vec3 reflect(const glm::vec3& I, const glm::vec3& N);
	void calculatePixel(int x, int y);
	void getTriangle(size_t triangleId, glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2);
	BoundingBox getBoundingBox(size_t triangleId);
	void expandBoundingBox(BoundingBox& bbox, size_t triangleId);

	void workerFunction();
	#ifdef THREADED
	void spawnWorkers();
	void joinWorkers();
	#endif

public:
	void run(const std::string& sceneName);
	void killThreads();
};