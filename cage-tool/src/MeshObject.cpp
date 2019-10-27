#include "MeshObject.h"

MeshObject::MeshObject() :
	vao(0), vertexBuffer(0),
	normalBuffer(0), uvBuffer(0),
	indexBuffer(0), textureID(0), hasTexture(false) {

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