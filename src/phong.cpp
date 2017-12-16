#include "phong.h"

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

			if (index.normal_index != -1) {
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
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

void Phong::buildKdTree() {
	std::vector<Triangle> triangles(vertices.size() / 3);

	for (size_t i=0; i<triangles.size(); ++i) {
		triangles[i] = { &transformed_vertices[3 * i], &transformed_vertices[3 * i + 1], &transformed_vertices[3 * i + 2] };
	}

	kdTree = KdNode::buildKdNode(triangles.data(), triangles.size(), 0);
}

void Phong::calculatePixel(int x, int y) {
	glm::vec3 dir = glm::normalize(glm::vec3{x, y, 0} - camera);
	glm::vec3 color { 0.1, 0.1, 0.1 };
	Ray ray { camera, dir };
	
	HitInfo hitInfo;
	if (ray.intersectKdNode(kdTree.get(), hitInfo)) {
		glm::vec3 normal = hitInfo.triangle->v0->normal * (1 - hitInfo.u - hitInfo.v) + hitInfo.triangle->v1->normal * hitInfo.u + hitInfo.triangle->v2->normal * hitInfo.v;
		glm::vec3 hitPoint = camera + dir * hitInfo.t;
		glm::vec3 diffuse, specular;

		for (const Light& light : transformed_lights) {
			glm::vec3 lightDir = glm::normalize(hitPoint - light.pos);
			glm::vec3 reflection = glm::reflect(lightDir, normal);

			diffuse += albedo * light.color * std::max(0.0f, glm::dot(normal, - lightDir));
			specular += light.color * std::pow(std::max(0.0f, glm::dot(reflection, -dir)), n);
		}

		color = albedo * ambient + diffuse * Kd + specular * Ks;
	}

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

	std::cout << "Building KdTree..." << std::endl;
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
