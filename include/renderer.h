#pragma once

#define BARYCENTER_INTERPOLATION

#include <csignal>
#include <cmath>

#include <iostream>
#include <fstream>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image_write.h"
#include "tiny_obj_loader.h"

#include "boundingbox.h"
#include "triangle.h"
#include "hitinfo.h"
#include "vertex.h"
#include "kdnode.h"
#include "light.h"
#include "ray.h"

class Renderer {
private:
	std::vector<Vertex> vertices;
	std::vector<Vertex> transformed_vertices;

	std::vector<Light> lights;
	std::vector<Light> transformed_lights;

	std::vector<uint32_t> imageData;

	uint32_t width;
	uint32_t height;
	size_t pixelCount;

	glm::mat4 model;
	glm::mat4 view;

	glm::vec3 albedo;
	float Kd;
	float Ks;
	float n;
	glm::vec3 ambient;

	glm::vec3 camera;

	constexpr static float epsilon = 1e-8f;

	std::atomic_size_t drawingIndex;

	std::unique_ptr<KdNode> kdTree;

	unsigned threadCount = 1;
	std::mutex coutMutex;
	std::vector<std::thread> workers;

	void loadModel(const std::string& modelName);
	void loadScene(const std::string& sceneFileName);
	void applyTransformation();
	void buildKdTree();
	void calculatePixel(uint32_t x, uint32_t y);

	void workerFunction();
	void spawnWorkers();
	void joinWorkers();

public:
	void run(const std::string& sceneName);
	void killThreads();
	void setThreadCount(unsigned threadCount);
};
