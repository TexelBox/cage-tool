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
	RenderEngine(GLFWwindow* window, Camera* camera);

	void render(const std::vector<MeshObject*>& objects);
	void renderLight();
	void assignBuffers(MeshObject& object);
	void setWindowSize(int width, int height);

	void updateLightPos(glm::vec3 add);

	unsigned int loadTexture(std::string filename);

private:
	GLFWwindow* window;
	Camera* camera;

	GLuint mainProgram;
	GLuint lightProgram;

	glm::mat4 projection;
	glm::vec3 lightPos;
};
