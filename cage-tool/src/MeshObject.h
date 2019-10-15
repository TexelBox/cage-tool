#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <math.h>

// Loads and stores (potentially textured) 3D meshes from .obj files. 
class MeshObject {

public:
	MeshObject();
	virtual ~MeshObject();

	std::vector<glm::vec3> verts;
	std::vector<glm::vec3> drawVerts;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<GLushort> faces;
	std::vector<GLushort> drawFaces;

	GLuint vao;
	GLuint vertexBuffer;
	GLuint normalBuffer;
	GLuint uvBuffer;
	GLuint indexBuffer;
	GLuint textureID;

	bool hasTexture;
};