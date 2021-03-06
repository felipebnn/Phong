#include "renderer.h"

#include <ctime>

void Renderer::loadModel(const std::string& modelName) {
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
				attrib.vertices[3 * static_cast<size_t>(index.vertex_index) + 0],
				attrib.vertices[3 * static_cast<size_t>(index.vertex_index) + 1],
				attrib.vertices[3 * static_cast<size_t>(index.vertex_index) + 2]
			};

			if (index.normal_index != -1) {
				vertex.normal = {
					attrib.normals[3 * static_cast<size_t>(index.normal_index) + 0],
					attrib.normals[3 * static_cast<size_t>(index.normal_index) + 1],
					attrib.normals[3 * static_cast<size_t>(index.normal_index) + 2]
				};
			}

			vertices.push_back(vertex);
		}
	}

	#ifdef BARYCENTER_INTERPOLATION
	if (shapes.size() > 0 && shapes[0].mesh.indices.size() > 0 && shapes[0].mesh.indices[0].normal_index == -1)
	#endif
	{
		#ifdef BARYCENTER_INTERPOLATION
		std::cerr << "No normals on the model!!" << std::endl;
		#endif

		for (size_t i=0; i<vertices.size(); i+=3) {
			Vertex& v0 = vertices[i];
			Vertex& v1 = vertices[i+1];
			Vertex& v2 = vertices[i+2];

			v0.normal = v1.normal = v2.normal = glm::normalize(glm::cross(v1.pos - v0.pos, v2.pos - v0.pos));
		}
	}
}

void Renderer::loadScene(const std::string& sceneFileName) {
	std::ifstream sceneFile(sceneFileName);

	if (!sceneFile) {
		throw std::runtime_error("failed to open file '" + sceneFileName + "'!");
	}

	std::string name;

	while (sceneFile >> name) {
		if (name == "size") {
			sceneFile >> width >> height;
			pixelCount = width * height;

			model = glm::translate(model, glm::vec3{.5f * width, -.5f * height, 0});
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

void Renderer::applyTransformation() {
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

void Renderer::buildKdTree() {
	std::vector<Triangle> triangles(vertices.size() / 3);

	for (size_t i=0, j=0; i<vertices.size(); i += 3) {
		triangles[j++] = { &transformed_vertices[i], &transformed_vertices[i + 1], &transformed_vertices[i + 2] };
	}

	kdTree = std::make_unique<KdNode>(triangles.begin(), triangles.end());
}

uint32_t Renderer::calculatePixel(uint32_t x, uint32_t y) {
	glm::vec3 dir = glm::normalize(glm::vec3{x, y, 0} - camera);
	glm::vec3 color { 0.1, 0.1, 0.1 };
	Ray ray { camera, dir };

	HitInfo hitInfo;
	if (ray.intersectKdNode(kdTree.get(), hitInfo)) {
		glm::vec3 normal = hitInfo.triangle->v0->normal * (1 - hitInfo.u - hitInfo.v) + hitInfo.triangle->v1->normal * hitInfo.u + hitInfo.triangle->v2->normal * hitInfo.v;
		glm::vec3 hitPoint = camera + dir * hitInfo.t;
		glm::vec3 diffuse {}, specular {};

		for (const Light& light : transformed_lights) {
			glm::vec3 lightDir = glm::normalize(hitPoint - light.pos);
			glm::vec3 reflection = glm::reflect(lightDir, normal);

			diffuse += albedo * light.color * std::max(0.0f, glm::dot(normal, - lightDir));
			specular += light.color * std::pow(std::max(0.0f, glm::dot(reflection, -dir)), n);
		}

		color = albedo * ambient + diffuse * Kd + specular * Ks;
	}

	glm::vec<3, uint32_t> colorInt;

	for (int j=0; j<3; ++j) {
		colorInt[j] = color[j] > 1.f ? 255 : static_cast<uint32_t>(color[j] * 255);
	}

	return colorInt.r | (colorInt.g << 8) | (colorInt.b << 16) | (0xFFu << 24);
}

void Renderer::workerFunction() {
	while (true) {
		size_t currentIndex = drawingIndex.fetch_add(1);

		if (currentIndex % (pixelCount / 100) == 0) {
			std::lock_guard<std::mutex> lg(coutMutex);
			std::cout << "\rRender process: " << 100 * currentIndex / pixelCount << "%";
			std::cout.flush();
		}

		if (currentIndex >= pixelCount) {
			break;
		}

		imageData[currentIndex] = calculatePixel(currentIndex % width, static_cast<uint32_t>(currentIndex / width));
	}
}

void Renderer::spawnWorkers() {
	std::cout << "Spawning " << threadCount - 1 << " extra workers..." << std::endl;

	workers.resize(0);
	for (size_t i=1; i<threadCount; ++i) {
		workers.emplace_back(&Renderer::workerFunction, this);
	}
}

void Renderer::joinWorkers() {
	for (std::thread& t : workers) {
		t.join();
	}
}

void Renderer::run(const std::string& sceneName) {
	clock_t startTime = clock();
	std::cout << "Rendering " << sceneName << "..." << std::endl;

	model = glm::mat4(1);
	view = glm::mat4(1);

	loadScene("scenes/" + sceneName + ".txt");
	clock_t loadTime = clock();

	std::cout << vertices.size() / 3 << " triangles..." << std::endl;

	applyTransformation();
	clock_t transformationTime = clock();

	std::cout << "Building KdTree..." << std::endl;
	buildKdTree();
	clock_t buildTime = clock();

	drawingIndex = 0;

	spawnWorkers();
	workerFunction();
	joinWorkers();
	clock_t rayTime = clock();

	std::cout << std::endl << std::endl;

	stbi_write_bmp(("images/" + sceneName + ".bmp").c_str(), static_cast<int32_t>(width), static_cast<int32_t>(height), 4, imageData.data());
	clock_t endTime = clock();

	std::cout << "Scene loading took " << (loadTime - startTime) << " milliseconds..." << std::endl;
	std::cout << "Transformations took " << (transformationTime - loadTime) << " milliseconds..." << std::endl;
	std::cout << "KdTree building took " << (buildTime - transformationTime) << " milliseconds..." << std::endl;
	std::cout << "RayTracing took " << (rayTime - buildTime) << " milliseconds..." << std::endl;
	std::cout << "Total time was " << (endTime - startTime) << " milliseconds..." << std::endl;
}

void Renderer::killThreads() {
	drawingIndex = pixelCount;
}

void Renderer::setThreadCount(unsigned threadCount) {
	this->threadCount = threadCount;
}
