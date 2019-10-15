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
	GLFWwindow* window;
	RenderEngine* renderEngine;
	Camera* camera;

	std::vector<MeshObject*> meshObjects;

	bool show_test_window;
	ImVec4 clear_color;

	//Geometry testObject;

	static void error(int error, const char* description);
	void setupWindow();
	void mainLoop();
	void drawUI();

	void createTestMeshObject();

	int counter;
};
