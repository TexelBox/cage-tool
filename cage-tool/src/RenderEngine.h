#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>

#include "Camera.h"
#include "MeshObject.h"
#include "ShaderTools.h"
#include "Texture.h"

#include "lodepng.h"

class RenderEngine {

public:
	RenderEngine(GLFWwindow *window, std::shared_ptr<Camera> camera);

	void renderPicking(std::vector<std::shared_ptr<MeshObject>> const& objects);
	void render(std::vector<std::shared_ptr<MeshObject>> const& objects);
	//void renderLight();
	void assignBuffers(MeshObject &object);
	void updateBuffers(MeshObject &object, bool const updateVerts, bool const updateUVs, bool const updateNormals, bool const updateColours);

	void setWindowSize(int width, int height);

	void updateLightPos(glm::vec3 add);

	unsigned int loadTexture(std::string filename);

private:
	GLFWwindow *window = nullptr;
	std::shared_ptr<Camera> camera = nullptr;

	GLuint mainProgram;
	GLuint lightProgram;
	GLuint pickingProgram;

	glm::mat4 projection;
	glm::vec3 lightPos;
};

