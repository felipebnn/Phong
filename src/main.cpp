#include <cmath>

#include <iostream>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define CULLING

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

std::vector<Vertex> vertices;
std::vector<Light> lights;

float cameraZ;
int width;
int height;
float scale;

glm::vec3 albedo;
float Kd;
float Ks;
float n;

constexpr float epsilon = 1e-8;

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
				throw std::runtime_error("no normals on model");
			}

			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vertices.push_back(vertex);
		}
	}
}

void loadScene(const std::string& sceneName) {
	std::ifstream sceneFile(sceneName);

	if (!sceneFile) {
		throw std::runtime_error("failed to open file '" + sceneName + "'!");
	}

	std::string modelName;
	size_t lightCount;

	sceneFile >> width >> height >> modelName >> scale >> albedo.x >> albedo.y >> albedo.z >> Kd >> n >> cameraZ >> lightCount;
	Ks = 1 - Kd;

	lights.resize(lightCount);
	for (size_t i=0; i<lightCount; ++i) {
		sceneFile >> lights[i].pos.x >> lights[i].pos.y >> lights[i].pos.z;
		sceneFile >> lights[i].color.x >> lights[i].color.y >> lights[i].color.z;
	}

	loadModel(modelName);
}

bool rayTriangleIntersect(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v) {
	glm::vec3 v0v1 = v1 - v0;
	glm::vec3 v0v2 = v2 - v0;
	glm::vec3 pvec = glm::cross(dir, v0v2);
	float det = glm::dot(v0v1, pvec);

	#ifdef CULLING
	if (det < epsilon) return false;
	#else
	if (fabs(det) < epsilon) return false;
	#endif

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

int main(int argc, char const *argv[]) {
	if (argc == 1) {
		loadScene("scene.txt");
	} else {
		loadScene(std::string(argv[1]) + ".txt");
	}

	uint32_t data[width*height];

	glm::mat4 model = glm::scale({}, glm::vec3{1, 1, 1} * scale);
	glm::mat4 view = glm::scale({}, glm::vec3{-1, -1, 1});

	glm::mat4 mv = view * model;
	glm::mat4 normal_mv = glm::transpose(glm::inverse(mv));

	std::vector<Vertex> transformed_vertices(vertices.size());
	for (size_t i=0; i<vertices.size(); ++i) {
		const Vertex& v = vertices[i];
		Vertex& transformed_v = transformed_vertices[i];

		glm::vec4 p = mv * glm::vec4{v.pos, 1.0f};
		transformed_v.pos = p / p.w;

		p = normal_mv * glm::vec4{v.normal, 1.0f};
		transformed_v.normal = glm::normalize(p / p.w);
	}

	std::vector<Light> transformed_lights(lights.size());
	for (size_t i=0; i<lights.size(); ++i) {
		const Light& l = lights[i];
		Light& transformed_l = transformed_lights[i];

		transformed_l.pos = view * glm::vec4{l.pos, 1.0f};
		transformed_l.color = l.color;
	}

	glm::vec3 camera = glm::vec3{0.0f, 0.0f, cameraZ};

	for (int x=-width/2; x<width/2; ++x) {
		std::cout << "\rLine " << x+width/2+1 << " out of " << width << ".";
		std::cout.flush();

		for (int y=-height/2; y<height/2; ++y) {
			glm::vec3 color { 0.1, 0.1, 0.1 };

			float currentDistance = std::numeric_limits<float>::max();

			glm::vec3 ray = glm::normalize(glm::vec3{x, y, 0} - camera);

			for (size_t i=0; i<transformed_vertices.size(); i += 3) {
				const Vertex& v0 = transformed_vertices[i];
				const Vertex& v1 = transformed_vertices[i+1];
				const Vertex& v2 = transformed_vertices[i+2];
				float t, u, v;
				
				if (rayTriangleIntersect(camera, ray, v0.pos, v1.pos, v2.pos, t, u, v) && t > -epsilon && t < currentDistance) {
					float w = 1 - u - v;
					glm::vec3 hitPoint = camera + ray * t;
					glm::vec3 normal = glm::normalize(v0.normal * w + v1.normal * u + v2.normal * v);
					glm::vec3 diffuse, specular;

					for (const Light& light : transformed_lights) {
						glm::vec3 lightDir = glm::normalize(hitPoint - light.pos);
						glm::vec3 reflection = reflect(lightDir, normal);

						diffuse += albedo * light.color * std::max(0.0f, glm::dot(normal, - lightDir));
						specular += light.color * std::pow(std::max(0.0f, glm::dot(reflection, -ray)), n);
					}

					color = diffuse * Kd + specular * Ks;

					for (size_t j=0; j<3; ++j) {
						if (color[j] > 1) {
							color[j] = 1;
							std::cerr << "Color channel maxed out!\n";
						}
					}
					currentDistance = t;
				}
			}

			data[(x + width/2) + (y + height/2) * width] = int(color[0] * 255) | (int(color[1] * 255) << 8) | (int(color[2] * 255) << 16) | (0xFF << 24);
		}
	}

	std::cout << std::endl;

	if (argc == 1) {
		stbi_write_bmp("out.bmp", width, height, 4, data);
	} else {
		stbi_write_bmp((std::string(argv[1]) + ".bmp").c_str(), width, height, 4, data);
	}
}