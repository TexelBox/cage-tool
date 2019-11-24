#include "RenderEngine.h"

RenderEngine::RenderEngine(GLFWwindow *window, std::shared_ptr<Camera> camera) : window(window), camera(camera) {
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	trivialProgram = ShaderTools::compileShaders("shaders/trivial.vert", "shaders/trivial.frag");
	mainProgram = ShaderTools::compileShaders("shaders/main.vert", "shaders/main.frag");
	lightProgram = ShaderTools::compileShaders("shaders/light.vert", "shaders/light.frag");
	pickingProgram = ShaderTools::compileShaders("shaders/picking.vert", "shaders/picking.frag");

	//NOTE: currently placing the light at the top of the y-axis
	lightPos = glm::vec3(0.0f, 500.0f, 0.0f);
	//projection = glm::perspective(45.0f, (float)width / height, 0.01f, 100.0f);
	//projection = glm::perspective(45.0f, (float)width / height, 0.01f, 1000.0f);
	projection = glm::perspective(45.0f, (float)width / height, 0.01f, 2000.0f);


	// Set OpenGL state
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glPointSize(30.0f);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}


//NOTE: this currently will ignore visibility flag
//NOTE: if the model has been imported, the model gets rendered as well since it is needed for occlusion tests (all "occlusion" objects should be passed here along with cage, but currently only the model makes sense)
//NOTE: currently assuming the cage is passed in with polygon mode as POINT
//NOTE: currently assuming the cage is passed in with pickingColours bound to its colour buffer
void RenderEngine::renderPicking(std::vector<std::shared_ptr<MeshObject>> const& objects) {

	glm::mat4 const view = camera->getLookAt();

	//NOTE: must clear to white here in case user changed the clear colour for normal rendering (white is assigned as the colour of non-interest (picking was false))
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//NOTE: not sure if blending matters here (or even should be disabled?), but i'll keep this here to be consistent with the other rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(pickingProgram);

	for (std::shared_ptr<MeshObject const> o : objects) {
		glBindVertexArray(o->vao);

		glm::mat4 const mvp = projection * view * o->getModel();

		glUniformMatrix4fv(glGetUniformLocation(pickingProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
		glUniform1i(glGetUniformLocation(pickingProgram, "hasPickingColour"), o->pickingColours.size() > 0);

		// POINT, LINE or FILL...
		glPolygonMode(GL_FRONT_AND_BACK, o->m_polygonMode);

		glDrawElements(o->m_primitiveMode, o->drawFaces.size(), GL_UNSIGNED_INT, (void*)0);
		glBindVertexArray(0);
	}
}


// Called to render provided objects under view matrix
void RenderEngine::render(std::vector<std::shared_ptr<MeshObject>> const& objects) {

	glm::mat4 const view = camera->getLookAt();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(mainProgram);

	for (std::shared_ptr<MeshObject const> o : objects) {

		// don't render invisible objects...
		if (!o->m_isVisible) continue;

		glBindVertexArray(o->vao);

		Texture::bind2DTexture(mainProgram, o->textureID, std::string("image"));

		glm::mat4 const model = o->getModel();
		glm::mat4 const modelView = view * model;

		glUniformMatrix4fv(glGetUniformLocation(mainProgram, "modelView"), 1, GL_FALSE, glm::value_ptr(modelView));
		glUniformMatrix4fv(glGetUniformLocation(mainProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		//glUniform3fv(glGetUniformLocation(mainProgram, "lightPos"), 1, glm::value_ptr(lightPos));
		glUniform3fv(glGetUniformLocation(mainProgram, "lightPos"), 1, glm::value_ptr(camera->getPosition()));

		glUniform1i(glGetUniformLocation(mainProgram, "hasTexture"), o->hasTexture);
		glUniform1i(glGetUniformLocation(mainProgram, "hasNormals"), !o->normals.empty());

		// POINT, LINE or FILL...
		glPolygonMode(GL_FRONT_AND_BACK, o->m_polygonMode);

		glDrawElements(o->m_primitiveMode, o->drawFaces.size(), GL_UNSIGNED_INT, (void*)0);

		//HACK: for now to get the cage to also render as points
		//NOTE: these points will get rendered using the trivial shader (no shading)
		if (o->m_renderPoints) {
			glUseProgram(trivialProgram);

			glm::mat4 const mvp = projection * modelView;
			glUniformMatrix4fv(glGetUniformLocation(trivialProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

			glPolygonMode(GL_FRONT_AND_BACK, PolygonMode::POINT);
			glDrawElements(o->m_primitiveMode, o->drawFaces.size(), GL_UNSIGNED_INT, (void*)0);
			
			// switch back to main shader for next object
			glUseProgram(mainProgram);
		}

		glBindVertexArray(0);
		Texture::unbind2DTexture();
	}
	//renderLight();
}

// Renders the current position of the light as a point
/*
void RenderEngine::renderLight() {
	glUseProgram(lightProgram);

	glm::mat4 const view = camera->getLookAt();
	// Uniforms
	glUniformMatrix4fv(glGetUniformLocation(lightProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(lightProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(glGetUniformLocation(lightProgram, "lightPos"), 1, glm::value_ptr(lightPos));

	glDrawArrays(GL_POINTS, 0, 1);
}
*/

// Assigns and binds buffers for a Mesh Object - vertices, normals, UV coordinates, faces
void RenderEngine::assignBuffers(MeshObject &object) 
{
	std::vector<glm::vec3> &vertices = object.drawVerts;
	std::vector<glm::vec3> &normals = object.normals;
	std::vector<glm::vec2> &uvs = object.uvs;
	std::vector<glm::vec3> &colours = object.colours;
	std::vector<GLuint> &faces = object.drawFaces;

	// Bind attribute array for triangles
	glGenVertexArrays(1, &object.vao);
	glBindVertexArray(object.vao);

	// Vertex buffer
	//NOTE: every object should have verts
	// location 0 in vao
	glGenBuffers(1, &object.vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, object.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	// Normal buffer
	// location 1 in vao
	if (normals.size() > 0) {
		glGenBuffers(1, &object.normalBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, object.normalBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*normals.size(), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(1);
	}

	if (uvs.size() > 0) {
		// UV buffer
		// location 2 in vao
		glGenBuffers(1, &object.uvBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, object.uvBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*uvs.size(), uvs.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(2);
	}

	// Colour buffer
	// location 3 in vao
	if (colours.size() > 0) {
		glGenBuffers(1, &object.colourBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, object.colourBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*colours.size(), colours.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(3);
	}
	
	// Face buffer
	//NOTE: assuming every object is using an index buffer (thus glDrawElements is always used)
	// this is fully compatible since if an object has only verts and wanted to use glDrawArrays, then it could just initialize a trivial index buffer (0,1,2,...,verts.size()-1)
	glGenBuffers(1, &object.indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*faces.size(), faces.data(), GL_STATIC_DRAW);
	

	// unbind vao
	glBindVertexArray(0);
}


//NOTE: this method assumes that the vector sizes have remained the same, the data in them has just changed
//NOTE: it also assumes that the buffers have already been created and bound to the vao (by assignBuffers)
void RenderEngine::updateBuffers(MeshObject &object, bool const updateVerts, bool const updateUVs, bool const updateNormals, bool const updateColours) {
	
	// nothing bound
	if (0 == object.vao) return;


	if (updateVerts && 0 != object.vertexBuffer) {
		std::vector<glm::vec3> const& newVerts = object.drawVerts;
		unsigned int const newSize = sizeof(glm::vec3)*newVerts.size();

		GLint oldSize = 0;
		glBindBuffer(GL_ARRAY_BUFFER, object.vertexBuffer);
		glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &oldSize); // get size of data in buffer

		// only update buffer data if new data is same size, otherwise buffer will be unchanged
		if (newSize == oldSize) {
			glBufferSubData(GL_ARRAY_BUFFER, 0, newSize, newVerts.data());
		}
	}

	if (updateUVs && 0 != object.uvBuffer) {
		std::vector<glm::vec2> const& newUVs = object.uvs;
		unsigned int const newSize = sizeof(glm::vec2)*newUVs.size();

		GLint oldSize = 0;
		glBindBuffer(GL_ARRAY_BUFFER, object.uvBuffer);
		glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &oldSize); // get size of data in buffer

		// only update buffer data if new data is same size, otherwise buffer will be unchanged
		if (newSize == oldSize) {
			glBufferSubData(GL_ARRAY_BUFFER, 0, newSize, newUVs.data());
		}
	}

	if (updateNormals && 0 != object.normalBuffer) {
		std::vector<glm::vec3> const& newNormals = object.normals;
		unsigned int const newSize = sizeof(glm::vec3)*newNormals.size();

		GLint oldSize = 0;
		glBindBuffer(GL_ARRAY_BUFFER, object.normalBuffer);
		glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &oldSize); // get size of data in buffer

		// only update buffer data if new data is same size, otherwise buffer will be unchanged
		if (newSize == oldSize) {
			glBufferSubData(GL_ARRAY_BUFFER, 0, newSize, newNormals.data());
		}
	}

	if (updateColours && 0 != object.colourBuffer) {
		//NOTE: currently only 2 colour modes
		std::vector<glm::vec3> const& newColours = object.m_colourMode == ColourMode::NORMAL ? object.colours : object.pickingColours;
		unsigned int const newSize = sizeof(glm::vec3)*newColours.size();

		GLint oldSize = 0;
		glBindBuffer(GL_ARRAY_BUFFER, object.colourBuffer);
		glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &oldSize); // get size of data in buffer

		// only update buffer data if new data is same size, otherwise buffer will be unchanged
		if (newSize == oldSize) {
			glBufferSubData(GL_ARRAY_BUFFER, 0, newSize, newColours.data());
		}
	}

}








// Creates a 2D texture
unsigned int RenderEngine::loadTexture(std::string filename) {
	//reading model texture image
	std::vector<unsigned char> _image;
	unsigned int _imageWidth, _imageHeight;

	unsigned int error = lodepng::decode(_image, _imageWidth, _imageHeight, filename.c_str());
	if (error)
	{
		std::cout << "reading error" << error << ":" << lodepng_error_text(error) << std::endl;
	}

	unsigned int id = Texture::create2DTexture(_image, _imageWidth, _imageHeight);
	return id;
}

// Updates lightPos by specified value
void RenderEngine::updateLightPos(glm::vec3 add) {
	lightPos += add;
}

// Sets projection and viewport for new width and height
void RenderEngine::setWindowSize(int width, int height) {
	projection = glm::perspective(45.0f, (float)width / height, 0.01f, 2000.0f);
	glViewport(0, 0, width, height);
}
