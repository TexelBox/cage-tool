#include "ObjectLoader.h"

#include <fstream>
#include <boost/algorithm/string.hpp>



//NOTE: this method simply returns the data as found in file (but with indices decremented by 1 for 0-indexing). Thus, for OpenGL, the data still needs to be converted into a single-index-buffer format.
// reference: https://www.cs.cmu.edu/~mbz/personal/graphics/obj.html
//NOTE: the referenced format above will be closely followed, although some of the information is innacurate (I think!) (e.g. f 1/1 is invalid, it would have to be f 1//1 or 1/1/)
//NOTE: this method does not support 3D texture coords (uvw) - will return false if found in file. Otherwise, 2D texture coords (uv) are allowed.
// reference: https://wiki.fileformat.com/3d/obj/
//NOTE: this loader is very incomplete according to above specification, but it is good enough for our application
//TODO: probably gonna have to keep track of stuff like "g" "usemtl", etc. and maybe comments so that they can be added to any exported files by our application (maybe we won't support that though...)
//TODO: could also return an error string with a specific error
// reference: http://paulbourke.net/dataformats/obj/
bool ObjectLoader::loadTriMeshOBJ(std::string const& filePath, std::vector<glm::vec3> &out_verts, std::vector<glm::vec2> &out_uvs, std::vector<glm::vec3> &out_normals, std::vector<std::vector<glm::vec3>> &out_faces) {

	// ERROR CHECKING...

	// verify extension is .obj (case-insensitive)
	// reference: https://stackoverflow.com/questions/51949/how-to-get-file-extension-from-string-in-c

	//NOTE: if find_last_of doesn't find any occurence, it returns std::string::npos 
	size_t const dotIndex = filePath.find_last_of(".");
	if (dotIndex == std::string::npos) return false;

	//NOTE: if dotIndex + 1 == filePath.length(), then substr will return ""
	//NOTE: if pos in substr(pos) is > string length, it will throw as exception. Below, this will never happen though due to the check above
	// performs case-insensitive comparison
	// reference: https://stackoverflow.com/questions/11635/case-insensitive-string-comparison-in-c
	if (!boost::iequals(filePath.substr(dotIndex + 1), "obj")) return false;


	// MAIN WORK...

	// 1. clear params (safety)...

	out_verts.clear();
	out_uvs.clear();
	out_normals.clear();
	out_faces.clear();

	// 2. read obj file line by line (each line as a string). Only error checking here will be format checking on the lines (e.g. f 1/1/1 2/2/2 3/3/3 4/4/4 would return false since we are assuming pure tri mesh)

	// open file
	std::ifstream fileStream;
	fileStream.open(filePath); // opens file for reading (default mode)
	// reference: https://stackoverflow.com/questions/4206816/ifstream-check-if-opened-successfully
	if (!fileStream) return false; // if opening failed or stream is bad (e.g. invalid path), error

	//NOTE: the file will be parsed in an order-agnostic manner
	std::string line = "";

	// reference: https://stackoverflow.com/questions/19123334/how-to-use-stdgetline-to-read-a-text-file-into-an-array-of-strings-in-c
	// reference: https://stackoverflow.com/questions/5605125/why-is-iostreameof-inside-a-loop-condition-i-e-while-stream-eof-cons
	while (std::getline(fileStream, line)) {

		boost::trim(line);
		if (line.empty()) continue; // ignore blank lines

		// reference: https://wiki.fileformat.com/3d/obj/
		//NOTE: this method will only be looking for v,vt,vn,f lines. Every line with a different prefix will simply be ignored (this includes all the valid prefix tokens listed in the above link, but it also unfortunately includes any gibberish as well)
		//NOTE: this seems fine by me since this code should work even if the file format is updated in the future (by adding features, not subtracting the ones we support)

		//NOTE: prefix tokens will be checked in a case-insensitive manner (idk if this is a violation of the spec, but its more flexible to the user)

		// reference: https://stackoverflow.com/questions/10551125/boost-string-split-to-eliminate-spaces-in-words

		
		//NOTE: command prefixes must be followed by a space character (or tab)
		// reference: https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
		std::replace(line.begin(), line.end(), '\t', ' '); // replace any and all tabs with simple space chars


		if (boost::istarts_with(line, "v ")) {
			std::string suffix = line.substr(2);
			//NOTE: this suffix cannot be empty since it would mean our whole trimmed line is "v " which is impossible since we trimmed our raw line to get it
			boost::trim_left(suffix); //NOTE: right trim is unneeded since line was already trimmed before
			//NOTE: this suffix is also guaranteed to not be empty (otherwise, it would have been trimmed before)

			//NOTE: suffix is expected to be in the format "x[whitespace]y[whitespace]z" where x,y,z are real numbers (will get parsed into floats)
			std::vector<std::string> words;
			boost::split(words, suffix, boost::is_any_of(" "), boost::token_compress_on); //NOTE: no need to check for '\t', since we already replaced them with ' ' in whole line

			if (words.size() != 3) return false; // invalid format

			//TODO: test -ve, none, +ve, ints, does 0.5f work (probably not, but no file will ever have this)
			try {
				glm::vec3 vert;
				vert.x = std::stof(words.at(0));
				vert.y = std::stof(words.at(1));
				vert.z = std::stof(words.at(2));
				out_verts.push_back(vert);
			} catch (...) { // catch-all (cannot convert or range violation)
				return false;
			}

		} else if (boost::istarts_with(line, "vt ")) {
			std::string suffix = line.substr(3);
			//NOTE: this suffix cannot be empty since it would mean our whole trimmed line is "vt " which is impossible since we trimmed our raw line to get it
			boost::trim_left(suffix); //NOTE: right trim is unneeded since line was already trimmed before
			//NOTE: this suffix is also guaranteed to not be empty (otherwise, it would have been trimmed before)

			//NOTE: suffix is expected to be in the format "u[whitespace]v" where u,v are real numbers (will get parsed into floats)
			//NOTE: if u,v,w is encountered, we treat it as a parsing error (this parser does not support 3D texture coords despite them being valid in specification)
			std::vector<std::string> words;
			boost::split(words, suffix, boost::is_any_of(" "), boost::token_compress_on); //NOTE: no need to check for '\t', since we already replaced them with ' ' in whole line

			if (words.size() != 2) return false; // invalid format

			//TODO: test -ve, none, +ve, ints, does 0.5f work (probably not, but no file will ever have this)
			try {
				glm::vec2 uv;
				uv.x = std::stof(words.at(0));
				uv.y = std::stof(words.at(1));
				out_uvs.push_back(uv);
			}
			catch (...) { // catch-all (cannot convert or range violation)
				return false;
			}

		} else if (boost::istarts_with(line, "vn ")) {
			std::string suffix = line.substr(3);
			//NOTE: this suffix cannot be empty since it would mean our whole trimmed line is "vn " which is impossible since we trimmed our raw line to get it
			boost::trim_left(suffix); //NOTE: right trim is unneeded since line was already trimmed before
			//NOTE: this suffix is also guaranteed to not be empty (otherwise, it would have been trimmed before)

			//NOTE: suffix is expected to be in the format "x[whitespace]y[whitespace]z" where x,y,z are real numbers (will get parsed into floats)
			std::vector<std::string> words;
			boost::split(words, suffix, boost::is_any_of(" "), boost::token_compress_on); //NOTE: no need to check for '\t', since we already replaced them with ' ' in whole line

			if (words.size() != 3) return false; // invalid format

			//TODO: test -ve, none, +ve, ints, does 0.5f work (probably not, but no file will ever have this)
			try {
				glm::vec3 normal;
				normal.x = std::stof(words.at(0));
				normal.y = std::stof(words.at(1));
				normal.z = std::stof(words.at(2));
				out_normals.push_back(normal);
			}
			catch (...) { // catch-all (cannot convert or range violation)
				return false;
			}

		} else if (boost::istarts_with(line, "f ")) {
			std::string suffix = line.substr(2);
			//NOTE: this suffix cannot be empty since it would mean our whole trimmed line is "f " which is impossible since we trimmed our raw line to get it
			boost::trim_left(suffix); //NOTE: right trim is unneeded since line was already trimmed before
			//NOTE: this suffix is also guaranteed to not be empty (otherwise, it would have been trimmed before)

			//NOTE: suffix is expected to be in the format "v0/vt0/vn0[whitespace]v1/vt1/vn1[whitespace]v2/vt2/vn2" where each token is an index (int >= 1) - NOTE: negative indices are not supported by this parser despite being valid in spec
			//NOTE: vt/vn are optional, v is required, thus if both are missing the line could look like v0 v1 v2 or v0// v1// v2//
			//NOTE: if an index is missing, a symbolic -1 will be put in its place
			//NOTE: this parser only works for pure tri meshes
			//NOTE: later on, I will error check that all explicit indices are in the proper format (e.g. can't have f 1/1/1 2//2 3/3/3 in the file)
			std::vector<std::string> words;
			boost::split(words, suffix, boost::is_any_of(" "), boost::token_compress_on); //NOTE: no need to check for '\t', since we already replaced them with ' ' in whole line

			if (words.size() != 3) return false; // invalid format

			std::vector<glm::vec3> points; // will be size 3 (triangle) - each of the 3 points has 3 indices (v/vt/vn)

			for (std::string const& point : words) {
				std::vector<std::string> indices;
				boost::split(indices, point, boost::is_any_of("/"), boost::token_compress_off);

				int vIndex = -1;
				int vtIndex = -1;
				int vnIndex = -1;

				if (indices.size() == 1) { // point is just a vIndex (no slashes)
					try {
						vIndex = std::stoi(indices.at(0));
						if (1 > vIndex) return false; // we only support indices >= 1
						--vIndex; // must decrement obj indices to shift to 0-indexing
					} catch (...) { // catch-all (cannot convert or range violation)
						return false;
					}
				} else if (indices.size() == 3) {
					try {
						if (indices.at(0).empty()) return false; // missing vIndex (NOTE: we require all faces to have a vIndex)
						vIndex = std::stoi(indices.at(0));
						if (1 > vIndex) return false; // we only support indices >= 1
						--vIndex; // must decrement obj indices to shift to 0-indexing
						
						if (!indices.at(1).empty()) { // empty vt will stay at -1
							vtIndex = std::stoi(indices.at(1));
							if (1 > vtIndex) return false; // we only support indices >= 1
							--vtIndex; // must decrement obj indices to shift to 0-indexing
						}

						if (!indices.at(2).empty()) { // empty vn will stay at -1
							vnIndex = std::stoi(indices.at(2));
							if (1 > vnIndex) return false; // we only support indices >= 1
							--vnIndex; // must decrement obj indices to shift to 0-indexing
						}
					} catch (...) { // catch-all (cannot convert or range violation)
						return false;
					}
				} else return false; // have slashes but don't have exactly 2

				// getting here means we have (so far) a valid triple of indices for this point
				points.push_back(glm::vec3(vIndex, vtIndex, vnIndex));
			}

			out_faces.push_back(points);

		} else {
			if ("v" == line || "vt" == line || "vn" == line || "f" == line) return false; // valid and supported prefix but, missing data
			else continue; //NOTE: this case handles both gibberish lines (incl. something like v10.2 10.5 12.1 - missing space after 'v') and lines prefixed by valid obj commands that we do not support here
		}

		// otherwise, continue as normal
	}

	// FILE HAS BEEN READ WITHOUT ERROR
	// POST ERROR CHECKING...

	//NOTE: file must have contained verts and faces
	//NOTE: this error could also happen if file was empty or gibberish
	if (out_verts.size() == 0 || out_faces.size() == 0) return false;

	//NOTE: all points (making up all faces) in the file must be in the same index format (e.g. v0 v1 v2 == v0// v1// v2//, but v0/vt1/ ... != v0//vn0 ...)
	//NOTE: only need to check the state of vtIndex/vnIndex since error checking in the f-section already made sure every point had a valid vIndex.
	//NOTE: also must check if every index in in range of their respective vector

	// first, we can figure out the format to look for based on the first point
	glm::vec3 const& firstPoint = out_faces.at(0).at(0);
	bool const vtIndexExpected = -1 != firstPoint.y;
	bool const vnIndexExpected = -1 != firstPoint.z;

	// loop through all faces...
	for (std::vector<glm::vec3> const& f : out_faces) {
		for (glm::vec3 const& p : f) {

			// 1. check index format is consistent...

			bool const vtFound = -1 != p.y;
			if (vtIndexExpected != vtFound) return false; // mismatch of index format

			bool const vnFound = -1 != p.z;
			if (vnIndexExpected != vnFound) return false; // mismatch of index format

			// 2. check each index is in respective vector range...
			//NOTE: only need to check if index exists (!= -1)

			if (out_verts.size() <= p.x) return false;
			if (vtFound && out_uvs.size() <= p.y) return false;
			if (vnFound && out_normals.size() <= p.z) return false;
		}
	}

	return true;
}



std::shared_ptr<MeshObject> ObjectLoader::createTriMeshObject(std::string const& filePath, bool const ignoreUVS, bool const ignoreNormals) {
	std::vector<glm::vec3> parsedVerts;
	std::vector<glm::vec2> parsedUVs;
	std::vector<glm::vec3> parsedNormals;
	std::vector<std::vector<glm::vec3>> parsedFaces;

	if (!loadTriMeshOBJ(filePath, parsedVerts, parsedUVs, parsedNormals, parsedFaces)) return nullptr; // parsing error

	//NOTE: guaranteed to have verts and faces by parser (if obj file is valid). UVs and Normals may not be found in file.
	//NOTE: the parser guarantees that we have a pure tri mesh
	//NOTE: an obj file with faces that don't specify uvs or normals or both, but the file still contains vt or vn lines is valid (we just have to ignore this extra data provided to us)

	// 1. can look at format of a point (they are all the same format) to figure out what data each face is made up of...
	glm::vec3 const& firstPoint = parsedFaces.at(0).at(0);
	bool const vtFound = -1 != firstPoint.y;
	bool const vnFound = -1 != firstPoint.z;

	// 2. now based on the found flags and our params, figure out what data needs to be extracted into a fresh MeshObject...
	//NOTE: verts and faces will always be included no matter what

	bool const includeUVs = vtFound && !ignoreUVS;
	bool const includeNormals = vnFound && !ignoreNormals;

	// 3. process the parsed data into an OpenGL single-index-buffer compatible format...

	//std::shared_ptr<MeshObject> triMesh = std::make_shared<MeshObject>();

	std::vector<glm::vec3> drawVerts;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<GLuint> drawFaces;


	// 4 CASES...

	//NOTE: remember that the parser won't return false if the file has unreferenced data (e.g. a v line whose index is never mentioned in any face) - thus, below we must only add referenced data to meshobject
	//NOTE: the parser also doesn't check each section for unique data (e.g. are all v lines unique?), but this is assumed in every obj file. - anyway, we we extract unique data anyway

	if (!includeUVs && !includeNormals) { // TRIVIAL CASE (just verts and faces)

		std::vector<int> vIndices; // stores unique singles

		for (std::vector<glm::vec3> const& f : parsedFaces) {
			for (glm::vec3 const& p : f) {

				int const vIndex = p.x;

				auto it = std::find(vIndices.begin(), vIndices.end(), vIndex);
				if (vIndices.end() != it) { // duplicate single
					unsigned int const index = it - vIndices.begin();
					drawFaces.push_back(index);
				} else { // new single
					vIndices.push_back(vIndex);
					drawVerts.push_back(parsedVerts.at(vIndex));
					drawFaces.push_back(vIndices.size() - 1);
				}
			}
		}

	} else if (includeUVs && !includeNormals) { // verts, uvs and faces

		std::vector<glm::vec2> v_vtIndexPairs; // stores unique pairs

		for (std::vector<glm::vec3> const& f : parsedFaces) {
			for (glm::vec3 const& p : f) {

				glm::vec2 const pair = glm::vec2(p.x, p.y);

				auto it = std::find(v_vtIndexPairs.begin(), v_vtIndexPairs.end(), pair);
				if (v_vtIndexPairs.end() != it) { // duplicate pair
					unsigned int const index = it - v_vtIndexPairs.begin();
					drawFaces.push_back(index);
				} else { // new pair
					v_vtIndexPairs.push_back(pair);
					drawVerts.push_back(parsedVerts.at(pair.x));
					uvs.push_back(parsedUVs.at(pair.y));
					drawFaces.push_back(v_vtIndexPairs.size() - 1);
				}
			}
		}

	} else if (!includeUVs && includeNormals) { // verts, normals and faces

		std::vector<glm::vec2> v_vnIndexPairs; // stores unique pairs

		for (std::vector<glm::vec3> const& f : parsedFaces) {
			for (glm::vec3 const& p : f) {

				glm::vec2 const pair = glm::vec2(p.x, p.z);

				auto it = std::find(v_vnIndexPairs.begin(), v_vnIndexPairs.end(), pair);
				if (v_vnIndexPairs.end() != it) { // duplicate pair
					unsigned int const index = it - v_vnIndexPairs.begin();
					drawFaces.push_back(index);
				} else { // new pair
					v_vnIndexPairs.push_back(pair);
					drawVerts.push_back(parsedVerts.at(pair.x));
					normals.push_back(parsedNormals.at(pair.y));
					drawFaces.push_back(v_vnIndexPairs.size() - 1);
				}
			}
		}

	} else { // verts, uvs, normals and faces

		std::vector<glm::vec3> v_vt_vnIndexTriples; // stores unique triples

		for (std::vector<glm::vec3> const& f : parsedFaces) {
			for (glm::vec3 const& p : f) {

				glm::vec3 const triple = p;

				auto it = std::find(v_vt_vnIndexTriples.begin(), v_vt_vnIndexTriples.end(), triple);
				if (v_vt_vnIndexTriples.end() != it) { // duplicate triple
					unsigned int const index = it - v_vt_vnIndexTriples.begin();
					drawFaces.push_back(index);
				} else { // new triple
					v_vt_vnIndexTriples.push_back(triple);
					drawVerts.push_back(parsedVerts.at(triple.x));
					uvs.push_back(parsedUVs.at(triple.y));
					normals.push_back(parsedNormals.at(triple.z));
					drawFaces.push_back(v_vt_vnIndexTriples.size() - 1);
				}
			}
		}

	}

	std::shared_ptr<MeshObject> triMesh = std::make_shared<MeshObject>();
	triMesh->drawVerts = drawVerts;
	triMesh->uvs = uvs;
	triMesh->normals = normals;
	triMesh->drawFaces = drawFaces;

	// init vert colours (uniform light grey for now)
	for (unsigned int i = 0; i < triMesh->drawVerts.size(); ++i) {
		triMesh->colours.push_back(glm::vec3(0.8f, 0.8f, 0.8f));
	}

	if (triMesh->uvs.size() > 0) triMesh->hasTexture = true; //TODO: probably gonna remove this hasTexture field later on

	return triMesh;
}





// DEPRECATED BELOW ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*

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

*/

/*

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

*/

/*

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

*/

/*

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

*/