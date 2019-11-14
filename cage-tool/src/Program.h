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


//NOTE: even if we don't implement HC or GC, this is future proof
enum CoordinateTypes {
	MVC = 0,
	HC = 1,
	GC = 2,
	NUM_COORDINATE_TYPES
};


class Program {

public:
	static glm::vec3 const s_CAGE_UNSELECTED_COLOUR;
	static glm::vec3 const s_CAGE_SELECTED_COLOUR;

	Program();
	void start();

	std::shared_ptr<MeshObject> m_model = nullptr;
	std::shared_ptr<MeshObject> m_cage = nullptr;

	void selectCageVerts(unsigned int const startIndex, unsigned int const count);
	void unselectCageVerts(unsigned int const startIndex, unsigned int const count);
	void toggleCageVerts(unsigned int const startIndex, unsigned int const count);

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


	std::shared_ptr<MeshObject> m_yzPlane = nullptr;
	std::shared_ptr<MeshObject> m_xzPlane = nullptr;
	std::shared_ptr<MeshObject> m_xyPlane = nullptr;


	std::vector<std::vector<float>> m_vertWeights; // [i][j] represents the weight of cage vert j on model vert i
	//std::vector<std::vector<float>> m_normalWeights; // [i][j] represents the weight of cage face normal j on model vert i (only used for GC)

	void computeCageWeights();
	void deformModel();


	CoordinateTypes m_coordinateType = CoordinateTypes::MVC; // default is MVC

};
