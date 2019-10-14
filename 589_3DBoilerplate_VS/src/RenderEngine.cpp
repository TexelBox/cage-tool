#include "RenderEngine.h"

RenderEngine::RenderEngine(GLFWwindow* window, Camera* camera) : window(window), camera(camera) {
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	mainProgram = ShaderTools::compileShaders("shaders/main.vert", "shaders/main.frag");
	lightProgram = ShaderTools::compileShaders("./shaders/light.vert", "./shaders/light.frag");

	lightPos = glm::vec3(0.0, 2.0, 0.0);
	projection = glm::perspective(45.0f, (float)width / height, 0.01f, 100.0f);

	// Set OpenGL state
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glPointSize(30.0f);
	glClearColor(1.0f, 1.0f, 1.0f, 0.0);
}

// Called to render provided objects under view matrix
void RenderEngine::render(const std::vector<MeshObject*>& objects) {

	glm::mat4 view = camera->getLookAt();
	glm::mat4 model = glm::mat4();
	glm::mat4 modelView = view * model;

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(mainProgram);

	for (const MeshObject* o : objects) {
		glBindVertexArray(o->vao);

		Texture::bind2DTexture(mainProgram, o->textureID, std::string("image"));

		//glm::mat4 modelView = view * o->modelMatrix;
		glUniformMatrix4fv(glGetUniformLocation(mainProgram, "modelView"), 1, GL_FALSE, glm::value_ptr(modelView));
		glUniformMatrix4fv(glGetUniformLocation(mainProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3fv(glGetUniformLocation(mainProgram, "lightPos"), 1, glm::value_ptr(lightPos));
		glUniform1i(glGetUniformLocation(mainProgram, "hasTexture"), o->hasTexture);

		glDrawElements(GL_TRIANGLES, o->drawFaces.size(), GL_UNSIGNED_SHORT, (void*)0);
		glBindVertexArray(0);
		Texture::unbind2DTexture();
	}
	renderLight();
}

// Renders the current position of the light as a point
void RenderEngine::renderLight() {
	glUseProgram(lightProgram);

	glm::mat4 view = camera->getLookAt();
	// Uniforms
	glUniformMatrix4fv(glGetUniformLocation(lightProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(lightProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(glGetUniformLocation(lightProgram, "lightPos"), 1, glm::value_ptr(lightPos));

	glDrawArrays(GL_POINTS, 0, 1);
}

// Assigns and binds buffers for a Mesh Object - vertices, normals, UV coordinates, faces
void RenderEngine::assignBuffers(MeshObject& object) 
{
	std::vector<glm::vec3>& vertices = object.drawVerts;
	std::vector<glm::vec3>& normals = object.normals;
	std::vector<glm::vec2>& uvs = object.uvs;
	std::vector<GLushort>& faces = object.drawFaces;

	// Bind attribute array for triangles
	glGenVertexArrays(1, &object.vao);
	glBindVertexArray(object.vao);

	// Vertex buffer
	glGenBuffers(1, &object.vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, object.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	// Normal buffer
	glGenBuffers(1, &object.normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, object.normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*normals.size(), normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);

	if (object.hasTexture) {
		// UV buffer
		glGenBuffers(1, &object.uvBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, object.uvBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*uvs.size(), uvs.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(2);
	}

	// Face buffer
	glGenBuffers(1, &object.indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*faces.size(), faces.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
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
	projection = glm::perspective(45.0f, (float)width / height, 0.01f, 100.0f);
	glViewport(0, 0, width, height);
}
