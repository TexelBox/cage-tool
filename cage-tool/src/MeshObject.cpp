#include "MeshObject.h"

#include <glm/gtx/transform.hpp>

MeshObject::MeshObject() :
	vao(0), vertexBuffer(0),
	normalBuffer(0), uvBuffer(0), colourBuffer(0),
	indexBuffer(0), textureID(0), hasTexture(false) {

	updateModel(); // init model matrix
}

MeshObject::~MeshObject() {
	// Remove data from GPU
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteBuffers(1, &uvBuffer);
	glDeleteBuffers(1, &normalBuffer);
	glDeleteBuffers(1, &colourBuffer);
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


//NOTE: this assumes counter-clockwise winding of triangular faces
//NOTE: this method does not overwrite the normal buffer, it just overwrites the normal vector data
//TODO: could also save the face normals into a member vector as well, but since they aren't gonna be used anywhere i'm leaving them just as locals
void MeshObject::generateNormals() {
	if (PrimitiveMode::TRIANGLES != m_primitiveMode) return;

	// clear any old normal data...
	normals.clear();

	// init (per-vertex normals)...
	normals.resize(drawVerts.size(), glm::vec3(0.0f, 0.0f, 0.0f));

	// foreach triangle face in mesh...
	for (unsigned int f = 0; f < drawFaces.size(); f += 3) {
		// save the 3 vert indices making up face f...
		unsigned int const p1_index = drawFaces.at(f);
		unsigned int const p2_index = drawFaces.at(f + 1);
		unsigned int const p3_index = drawFaces.at(f + 2);

		// get the 3 vert positions...
		glm::vec3 const p1 = drawVerts.at(p1_index);
		glm::vec3 const p2 = drawVerts.at(p2_index);
		glm::vec3 const p3 = drawVerts.at(p3_index);

		// compute the outward face-normal (assuming CCW winding)...
		glm::vec3 const sideA = p2 - p1;
		glm::vec3 const sideB = p3 - p2;
		glm::vec3 const normal = glm::normalize(glm::cross(sideA, sideB));
		// SAFETY CHECK (e.g. if sideA or sideB were 0 vector OR if we tried to normalize the 0 vector)...
		// on error, this face has no contribution (could flag this face normal symbolically as the 0 vector)
		if (glm::any(glm::isnan(normal))) continue;

		//TODO: in the future, it would be better to weight this contribution by the face angle, but for now just doing the trivial average technique
		normals.at(p1_index) += normal;
		normals.at(p2_index) += normal;
		normals.at(p3_index) += normal;
	}

	// normalize the accumulated normals...
	for (glm::vec3 &n : normals) {
		n = glm::normalize(n);
		// SAFETY CHECK (e.g. if n was somehow -nan or 0 vector...)
		// set this normal symbolically as 0 vector
		if (glm::any(glm::isnan(n))) n = glm::vec3(0.0f, 0.0f, 0.0f);
	}

}
