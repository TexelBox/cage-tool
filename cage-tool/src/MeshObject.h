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

	void setPosition(glm::vec3 const position) { m_position = position; updateModel(); }
	void setRotation(glm::vec3 const rotation) { m_rotation = rotation; updateModel(); }
	void setScale(glm::vec3 const scale) { m_scale = scale; updateModel(); }

	glm::vec3 getPosition() const { return m_position; }
	glm::vec3 getRotation() const { return m_rotation; }
	glm::vec3 getScale() const { return m_scale; }

	glm::mat4 getModel() const { return m_model; }

private:

	// these will represent exactly the values seen by the user in the UI (thus we use degrees since they're more user-friendly)...
	glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 0.0f); // (x, y, z) position vector of object's origin point
	glm::vec3 m_rotation = glm::vec3(0.0f, 0.0f, 0.0f); // (x, y, z) rotation vector specified in euler angles (x degrees ccw around +x axis, y degrees ccw around +y axis, z degrees ccw around +z axis) 
	glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f); // (x, y, z) scale vector relative to object's origin point

	glm::mat4 m_model = glm::mat4(); // model matrix

	void updateModel(); // updates model matrix to reflect new state of m_position, m_rotation and m_scale
};