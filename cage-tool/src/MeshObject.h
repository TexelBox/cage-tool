#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <math.h>


enum PolygonMode {
	POINT = GL_POINT, // point-cloud
	LINE = GL_LINE, // wireframe
	FILL = GL_FILL, // full-faced
};


// Loads and stores (potentially textured) 3D meshes from .obj files. 
class MeshObject {

public:
	MeshObject();
	virtual ~MeshObject();

	//TODO: probably will have to change this to store better adjacency information to make our algorithms easier
	std::vector<glm::vec3> verts;
	std::vector<glm::vec3> drawVerts;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<GLuint> faces;
	std::vector<GLuint> drawFaces;

	GLuint vao;
	GLuint vertexBuffer;
	GLuint normalBuffer;
	GLuint uvBuffer;
	GLuint indexBuffer;
	GLuint textureID;

	bool hasTexture;

	PolygonMode m_polygonMode = PolygonMode::FILL; // default is to render full-faced (FILL), but can also render as wireframe (LINE) or as point-cloud (POINT)
};