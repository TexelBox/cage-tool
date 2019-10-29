#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

#include "Camera.h"
#include "InputHandler.h"
#include "MeshObject.h"
#include "ObjectLoader.h"
#include "RenderEngine.h"

class Program {

public:
	Program();
	void start();

private:
	GLFWwindow *window = nullptr;
	std::shared_ptr<RenderEngine> renderEngine = nullptr;
	std::shared_ptr<Camera> camera = nullptr;

	std::vector<std::shared_ptr<MeshObject>> meshObjects;

	ImVec4 clearColor;

	static void error(int error, char const* description);
	void setupWindow();
	void mainLoop();
	void drawUI();

	void createTestMeshObject();


	std::shared_ptr<MeshObject> m_model = nullptr;
	std::shared_ptr<MeshObject> m_cage = nullptr;


};
