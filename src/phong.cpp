#include "phong.h"

#include <algorithm>

void Phong::loadModel(const std::string& modelName) {
	vertices.resize(0);

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelName.c_str())) {
		throw std::runtime_error(err);
	}

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex {};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			if (index.normal_index == -1) {
				#ifdef BARYCENTER_INTERPOLATION
				throw std::runtime_error("no normals on model");
				#endif
			} else {
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			vertices.push_back(vertex);
		}
	}
}

void Phong::loadScene(const std::string& sceneFileName) {
	std::ifstream sceneFile(sceneFileName);

	if (!sceneFile) {
		throw std::runtime_error("failed to open file '" + sceneFileName + "'!");
	}

	std::string name;

	while (sceneFile >> name) {
		if (name == "size") {
			sceneFile >> width >> height;
			pixelCount = width * height;

			model = glm::translate(model, glm::vec3{width/2, -height/2, 0});
			imageData.resize(pixelCount);
		} else if (name == "model") {
			std::string modelName;
			sceneFile >> modelName;

			loadModel("models/" + modelName + ".obj");
		} else if (name == "scale") {
			float scale;
			sceneFile >> scale;

			model = glm::scale(model, glm::vec3{1, 1, 1} * scale);
		} else if (name == "translate") {
			glm::vec3 translation;
			sceneFile >> translation.x >> translation.y >> translation.z;

			model = glm::translate(model, translation);
		} else if (name == "rotate_x") {
			float angle;
			sceneFile >> angle;

			model = glm::rotate(model, glm::radians(angle), glm::vec3{1, 0, 0});
		} else if (name == "rotate_y") {
			float angle;
			sceneFile >> angle;

			model = glm::rotate(model, glm::radians(angle), glm::vec3{0, 1, 0});
		} else if (name == "rotate_z") {
			float angle;
			sceneFile >> angle;

			model = glm::rotate(model, glm::radians(angle), glm::vec3{0, 0, 1});
		} else if (name == "albedo") {
			sceneFile >> albedo.x >> albedo.y >> albedo.z;
		} else if (name == "kd") {
			sceneFile >> Kd;
		} else if (name == "ks") {
			sceneFile >> Ks;
		} else if (name == "n") {
			sceneFile >> n;
		} else if (name == "ambient") {
			sceneFile >> ambient.x >> ambient.y >> ambient.z;
		} else if (name == "cameraz") {
			float cameraZ;
			sceneFile >> cameraZ;

			camera = glm::vec3{width/2, height/2, cameraZ};
		} else if (name == "lights") {
			size_t lightCount;
			sceneFile >> lightCount;

			lights.resize(lightCount);
			for (size_t i=0; i<lightCount; ++i) {
				sceneFile >> lights[i].pos.x >> lights[i].pos.y >> lights[i].pos.z;
				sceneFile >> lights[i].color.x >> lights[i].color.y >> lights[i].color.z;
			}
		}
	}
}

void Phong::applyTransformation() {
	view = glm::scale(view, glm::vec3{1, -1, 1});

	glm::mat4 mv = view * model;
	glm::mat3 normal_mv = glm::transpose(glm::inverse(glm::mat3{mv}));

	transformed_vertices.resize(vertices.size());
	for (size_t i=0; i<vertices.size(); ++i) {
		const Vertex& v = vertices[i];
		Vertex& transformed_v = transformed_vertices[i];

		transformed_v.pos = mv * glm::vec4{v.pos, 1.0f};
		transformed_v.normal = glm::normalize(normal_mv * v.normal);
	}

	transformed_lights.resize(lights.size());
	for (size_t i=0; i<lights.size(); ++i) {
		const Light& l = lights[i];
		Light& transformed_l = transformed_lights[i];

		transformed_l.pos = view * glm::vec4{l.pos, 1.0f};
		transformed_l.color = l.color;
	}
}

std::unique_ptr<KdNode> Phong::buildKdNode(Triangle* triangles, size_t triangleCount, int depth) const {
	std::unique_ptr<KdNode> node = std::make_unique<KdNode>();
	node->triangles = triangles;
	node->triangleCount = triangleCount;
	node->bbox = {};

	if (triangleCount == 0) {
		return node;
	}

	node->bbox = BoundingBox::fromTriangle(triangles[0]); //TODO: bottom-up boundingbox
	for (size_t i=1; i<triangleCount; ++i) {
		node->bbox.expand(triangles[i]);
	}

	if (triangleCount < 2) {
		return node;
	}

	std::sort(triangles, triangles + triangleCount, [this, depth] (const Triangle& t0, const Triangle& t1) {
		glm::vec3 c0 = t0.v0->pos + t0.v1->pos + t0.v2->pos;
		glm::vec3 c1 = t1.v0->pos + t1.v1->pos + t1.v2->pos;

		switch (depth % 3) {
			case 0:
				return c0.x < c1.x;
			case 1:
				return c0.y < c1.y;
			case 2:
				return c0.z < c1.z;
		}

		return false;
	});

	Triangle* mid = triangles + triangleCount / 2;

	node->left = buildKdNode(triangles, mid - triangles, depth + 1);
	node->right = buildKdNode(mid, triangles + triangleCount - mid, depth + 1);

	return node;
}

void Phong::buildKdTree() {
	triangles.resize(vertices.size() / 3);

	for (size_t i=0; i<triangles.size(); ++i) {
		triangles[i] = { &transformed_vertices[3 * i], &transformed_vertices[3 * i + 1], &transformed_vertices[3 * i + 2] };
	}

	kdTree = buildKdNode(triangles.data(), triangles.size(), 0);
}

bool Phong::rayTriangleIntersect(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v) const {
	glm::vec3 v0v1 = v1 - v0;
	glm::vec3 v0v2 = v2 - v0;
	glm::vec3 pvec = glm::cross(dir, v0v2);
	float det = glm::dot(v0v1, pvec);

	if (det > -epsilon) return false; //culling

	float invDet = 1 / det;

	glm::vec3 tvec = orig - v0;
	u = glm::dot(tvec, pvec) * invDet;
	if (u < 0 || u > 1) return false;

	glm::vec3 qvec = glm::cross(tvec, v0v1);
	v = glm::dot(dir, qvec) * invDet;
	if (v < 0 || u + v > 1) return false;

	t = glm::dot(v0v2, qvec) * invDet;

	return true;
}

bool Phong::rayBoundingBoxIntersect(const glm::vec3& orig, const glm::vec3& dir, const BoundingBox& bbox) const {
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

bool Phong::rayKdNodeIntersection(KdNode* node, const glm::vec3& orig, const glm::vec3& dir, float& t, glm::vec3& color) const {
	if (rayBoundingBoxIntersect(orig, dir, node->bbox)) {
		if (node->left || node->right) {
			bool hitLeft = rayKdNodeIntersection(node->left.get(), orig, dir, t, color);
			bool hitRight = rayKdNodeIntersection(node->right.get(), orig, dir, t, color);
			return hitLeft || hitRight;
		} else {
			bool hit = false;
			float currT, u, v;

			for (size_t i=0; i<node->triangleCount; ++i) {
				const Vertex& v0 = *node->triangles[i].v0;
				const Vertex& v1 = *node->triangles[i].v1;
				const Vertex& v2 = *node->triangles[i].v2;

				if (rayTriangleIntersect(orig, dir, v0.pos, v1.pos, v2.pos, currT, u, v) && currT > -epsilon && currT < t) {
					color = {1, 1, 1};
					t = currT;

					#ifdef BARYCENTER_INTERPOLATION
					glm::vec3 normal = v0.normal * (1 - u - v) + v1.normal * u + v2.normal * v;
					#else
					glm::vec3 normal = glm::normalize(glm::cross(v2.pos - v0.pos, v1.pos - v0.pos));
					#endif

					glm::vec3 hitPoint = orig + dir * t;
					glm::vec3 diffuse, specular;

					for (const Light& light : transformed_lights) {
						glm::vec3 lightDir = glm::normalize(hitPoint - light.pos);
						glm::vec3 reflection = reflect(lightDir, normal);

						diffuse += albedo * light.color * std::max(0.0f, glm::dot(normal, - lightDir));
						specular += light.color * std::pow(std::max(0.0f, glm::dot(reflection, -dir)), n);
					}

					color = albedo * ambient + diffuse * Kd + specular * Ks;

					hit = true;
				}
			}

			return hit;
		}
	}
	return false;
}

glm::vec3 Phong::reflect(const glm::vec3& I, const glm::vec3& N) const {
    return I - 2 * glm::dot(I, N) * N;
}

void Phong::calculatePixel(int x, int y) {
	glm::vec3 color { 0.1, 0.1, 0.1 };

	glm::vec3 ray = glm::normalize(glm::vec3{x, y, 0} - camera);

	float t = std::numeric_limits<float>::max();
	rayKdNodeIntersection(kdTree.get(), camera, ray, t, color);

	for (size_t j=0; j<3; ++j) {
		if (color[j] > 1) {
			color[j] = 1;
		}
	}

	imageData[x + y * width] = int(color[0] * 255) | (int(color[1] * 255) << 8) | (int(color[2] * 255) << 16) | (0xFF << 24);
}

void Phong::workerFunction() {
	size_t currentIndex;

	while (true) {
		{
			#ifdef THREADED
			std::lock_guard<std::mutex> lg(jobsMutex);
			#endif

			if (drawingIndex % (pixelCount / 100) == 0) {
				std::cout << "\rRender process: " << 100 * drawingIndex / pixelCount << "%";
				std::cout.flush();
			}

			if (drawingIndex >= pixelCount) {
				break;
			}

			currentIndex = drawingIndex++;
		}

		calculatePixel(currentIndex % width, currentIndex / width);
	}
}

#ifdef THREADED
void Phong::spawnWorkers() {
	size_t threadCount = std::thread::hardware_concurrency();
	std::cout << "Spawning " << threadCount << " workers..." << std::endl;

	workers.resize(0);
	for (size_t i=0; i<threadCount; ++i) {
		workers.emplace_back(&Phong::workerFunction, this);
	}
}

void Phong::joinWorkers() {
	for (std::thread& t : workers) {
		t.join();
	}
}
#endif

void Phong::run(const std::string& sceneName) {
	std::cout << "Rendering " << sceneName << "..." << std::endl;

	model = {};
	view = {};

	loadScene("scenes/" + sceneName + ".txt");

	std::cout << vertices.size() / 3 << " triangles..." << std::endl;

	applyTransformation();

	buildKdTree();

	drawingIndex = 0;

	#ifdef THREADED
	spawnWorkers();
	joinWorkers();
	#else
	workerFunction();
	#endif

	std::cout << std::endl << std::endl;

	stbi_write_bmp(("images/" + sceneName + ".bmp").c_str(), width, height, 4, imageData.data());
}

void Phong::killThreads() {
	drawingIndex = -1;
}
