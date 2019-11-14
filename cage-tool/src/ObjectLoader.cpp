#include "ObjectLoader.h"

#include <boost/algorithm/string.hpp>

// Create MeshObject from obj file
std::shared_ptr<MeshObject> ObjectLoader::createMeshObject(std::string modelFile) {
	std::shared_ptr<MeshObject> m = std::make_shared<MeshObject>();
	std::vector<glm::vec3> verts;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	std::vector<GLuint> faces;
	std::vector<glm::vec3> raw_verts;

	ObjectLoader::loadOBJ(modelFile.c_str(), verts, uvs, normals, faces, raw_verts);

	std::vector<unsigned int> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	ObjectLoader::indexVBO(verts, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);
	m->drawVerts = indexed_vertices;
	m->verts = raw_verts;
	m->uvs = indexed_uvs;
	m->normals = indexed_normals;

	// init vert colours (uniform light grey for now)
	for (unsigned int i = 0; i < m->drawVerts.size(); ++i) {
		m->colours.push_back(glm::vec3(0.8f, 0.8f, 0.8f));
	}

	m->drawFaces = indices;
	m->faces = faces;

	if (indexed_uvs.size() > 0) m->hasTexture = true;

	return m;
}

bool ObjectLoader::getSimilarVertexIndex_fast(
	PackedVertex &packed,
	std::map<PackedVertex, unsigned int> &VertexToOutIndex,
	unsigned int &result)
{
	std::map<PackedVertex, unsigned int>::iterator it = VertexToOutIndex.find(packed);
	if (VertexToOutIndex.end() == it) {
		return false;
	}
	else {
		result = it->second;
		return true;
	}
}

void ObjectLoader::indexVBO(
	std::vector<glm::vec3> &in_vertices,
	std::vector<glm::vec2> &in_uvs,
	std::vector<glm::vec3> &in_normals,

	std::vector<unsigned int> &out_indices,
	std::vector<glm::vec3> &out_vertices,
	std::vector<glm::vec2> &out_uvs,
	std::vector<glm::vec3> &out_normals)
{
	std::map<PackedVertex, unsigned int> VertexToOutIndex;

	// For each input vertex
	for (unsigned int i = 0; i < in_vertices.size(); i++) {

		PackedVertex packed;
		if (in_uvs.size() > 0)
			packed = { in_vertices[i], in_uvs[i], in_normals[i] };

		else
			packed = { in_vertices[i], glm::vec2(), in_normals[i] };

		// Try to find a similar vertex in out_XXXX
		unsigned int index;
		bool found = getSimilarVertexIndex_fast(packed, VertexToOutIndex, index);

		if (found) { // A similar vertex is already in the VBO, use it instead !
			out_indices.push_back(index);
		}
		else { // If not, it needs to be added in the output data.
			out_vertices.push_back(in_vertices[i]);

			if (in_uvs.size() > 0)
				out_uvs.push_back(in_uvs[i]);

			out_normals.push_back(in_normals[i]);
			unsigned int newindex = (unsigned int)out_vertices.size() - 1;
			out_indices.push_back(newindex);
			VertexToOutIndex[packed] = newindex;
		}
	}
}

bool ObjectLoader::loadOBJ(
	char const* path,
	std::vector<glm::vec3> &out_vertices,
	std::vector<glm::vec2> &out_uvs,
	std::vector<glm::vec3> &out_normals,
	std::vector<GLuint> &out_faces,
	std::vector<glm::vec3> &raw_verts)
{
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	FILE *file = NULL;
	file = fopen(path, "r");
	if (NULL == file) {
		printf("Cannot open file. Check path.");
		getchar(); // block until user enters char
		return false;
	}

	bool hasTexture = false;

	while (true) {
		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (EOF == res) break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader

		if (0 == strcmp(lineHeader, "v")) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (0 == strcmp(lineHeader, "vt")) {
			hasTexture = true;
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y;
			temp_uvs.push_back(uv);
		}
		else if (0 == strcmp(lineHeader, "vn")) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (0 == strcmp(lineHeader, "f")) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];

			//NOTE: this code currently fails when trying to load files without normals. It should be extended to handle all valid OBJ specifications or you should document any assumptions
			if (hasTexture) {
				fscanf(file, "%u/%u/%u %u/%u/%u %u/%u/%u\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			}
			else {
				fscanf(file, "%u//%u %u//%u %u//%u\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2]);
			}

			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);

			if (hasTexture)
			{
				uvIndices.push_back(uvIndex[0]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[2]);
			}

			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
			out_faces.push_back(vertexIndex[0] - 1);
			out_faces.push_back(vertexIndex[1] - 1);
			out_faces.push_back(vertexIndex[2] - 1);
		}
		else {
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}
	raw_verts = temp_vertices;

	// For each vertex of each triangle
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		glm::vec3 normal = temp_normals[normalIndex - 1];

		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_normals.push_back(normal);

		if (uvIndices.size() > 0)
		{
			unsigned int uvIndex = uvIndices[i];
			glm::vec2 uv = temp_uvs[uvIndex - 1];
			out_uvs.push_back(uv);
		}

	}
	fclose(file);
	file = NULL;
	return true;
}