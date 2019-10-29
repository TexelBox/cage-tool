#include "MeshObject.h"

#include <glm/gtx/transform.hpp>

MeshObject::MeshObject() :
	vao(0), vertexBuffer(0),
	normalBuffer(0), uvBuffer(0),
	indexBuffer(0), textureID(0), hasTexture(false) {

	updateModel(); // init model matrix
}

MeshObject::~MeshObject() {
	// Remove data from GPU
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteBuffers(1, &uvBuffer);
	glDeleteBuffers(1, &normalBuffer);
	glDeleteBuffers(1, &indexBuffer);
	glDeleteVertexArrays(1, &vao);
	
	// delete the texture object since it never gets reused...
	glDeleteTextures(1, &textureID);
}


void MeshObject::updateModel() {

	// M = T*R*S
	// compute T, R, S first...
	glm::mat4 tMat = glm::translate(m_position);
	glm::mat4 rxMat = glm::rotate(glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 ryMat = glm::rotate(glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 rzMat = glm::rotate(glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 rMat = rxMat * ryMat * rzMat; // Z then Y then X
	glm::mat4 sMat = glm::scale(m_scale);

	m_model = tMat * rMat * sMat; // S then R then T
}