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

#include "stb_image_write.h"
#include "tiny_obj_loader.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
};

struct Triangle {
	size_t index1, index2, index3;
};

struct Light {
	glm::vec3 pos;
	glm::vec3 color;
};

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

	#ifdef THREADED
	std::mutex jobsMutex;
	std::vector<std::thread> workers;
	#endif

	void loadModel(const std::string& modelName) {
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

	void loadScene(const std::string& sceneFileName) {
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

	void applyTransformation() {
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

	bool rayTriangleIntersect(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v) {
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
		if (v < 0 || u + v > 1.001) return false;

		t = glm::dot(v0v2, qvec) * invDet;

		return true;
	}

	glm::vec3 reflect(const glm::vec3& I, const glm::vec3& N) {
	    return I - 2 * glm::dot(I, N) * N;
	}

	void calculatePixel(int x, int y) {
		glm::vec3 color { 0.1, 0.1, 0.1 };

		float currentDistance = std::numeric_limits<float>::max();

		glm::vec3 ray = glm::normalize(glm::vec3{x, y, 0} - camera);

		for (size_t i=0; i<transformed_vertices.size(); i += 3) {
			const Vertex& v0 = transformed_vertices[i];
			const Vertex& v1 = transformed_vertices[i+1];
			const Vertex& v2 = transformed_vertices[i+2];
			float t, u, v;
			
			if (rayTriangleIntersect(camera, ray, v0.pos, v1.pos, v2.pos, t, u, v) && t > -epsilon && t < currentDistance) {
				#ifdef BARYCENTER_INTERPOLATION
				glm::vec3 normal = v0.normal * (1 - u - v) + v1.normal * u + v2.normal * v;
				#else
				glm::vec3 normal = glm::normalize(glm::cross(v2.pos - v0.pos, v1.pos - v0.pos));
				#endif

				glm::vec3 hitPoint = camera + ray * t;
				glm::vec3 diffuse, specular;

				for (const Light& light : transformed_lights) {
					glm::vec3 lightDir = glm::normalize(hitPoint - light.pos);
					glm::vec3 reflection = reflect(lightDir, normal);

					diffuse += albedo * light.color * std::max(0.0f, glm::dot(normal, - lightDir));
					specular += light.color * std::pow(std::max(0.0f, glm::dot(reflection, -ray)), n);
				}

				color = albedo * ambient + diffuse * Kd + specular * Ks;

				currentDistance = t;
			}
		}
		
		for (size_t j=0; j<3; ++j) {
			if (color[j] > 1) {
				color[j] = 1;
			}
		}
		imageData[x + y * width] = int(color[0] * 255) | (int(color[1] * 255) << 8) | (int(color[2] * 255) << 16) | (0xFF << 24);
	}

	void workerFunction() {
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
	void spawnWorkers() {
		size_t threadCount = std::thread::hardware_concurrency();
		std::cout << "Spawning " << threadCount << " workers..." << std::endl;

		workers.resize(0);
		for (size_t i=0; i<threadCount; ++i) {
			workers.emplace_back(&Phong::workerFunction, this);
		}
	}

	void joinWorkers() {
		for (std::thread& t : workers) {
			t.join();
		}
	}
	#endif

public:
	void run(const std::string& sceneName) {
		std::cout << "Rendering " << sceneName << "..." << std::endl;

		model = {};
		view = {};

		loadScene("scenes/" + sceneName + ".txt");

		applyTransformation();

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

	void killThreads() {
		drawingIndex = -1;
	}
};

Phong p;
bool running = true;

int main(int argc, char const *argv[]) {
	signal(SIGINT, [] (int) { p.killThreads(); running = false; });

	if (argc == 1) {
		p.run("scene");
	} else {
		for (int i=1; i<argc && running; ++i) {
			std::string sceneName = argv[i];
			std::string extension = ".txt";

			if (std::equal(extension.rbegin(), extension.rend(), sceneName.rbegin())) {
				sceneName = sceneName.substr(0, sceneName.length() - 4);
			}

			size_t pos = sceneName.rfind('/');
			if (pos != std::string::npos) {
				sceneName = sceneName.substr(pos+1, std::string::npos);
			}

			p.run(sceneName);
		}
	}

}
