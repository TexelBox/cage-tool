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



struct MeshTree {
	std::vector<glm::vec3> m_vertexCoords; // .x (in V1 axis), .y (in V2 axis), .z (in V3 axis) - most will be integers, except for verts added for triangulation of cutting plane area
	std::vector<unsigned int> m_faceIndices; // 3 indices in a row correspond to a triangle face (CCW winding) of 3 verts in m_vertexCoords
};


//NOTE: m_axis should be normalized
//NOTE: m_max >= m_min is assumed
struct SortableAxis {
	glm::vec3 m_axis;
	float m_min;
	float m_max;

	SortableAxis(glm::vec3 const axis, float const min, float const max) : m_axis(axis), m_min(min), m_max(max) {}

	bool operator<(SortableAxis const& sa) const {
		return (m_max - m_min) < (sa.m_max - sa.m_min);
	}
};


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

	void translateSelectedCageVerts(glm::vec3 const& translation);

	float getDeltaMove() const {
		return m_deltaMove;
	}

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

	void initScene();
	void clearModel();
	void clearCage();
	void loadModel(std::string const& filePath);
	void loadCage(std::string const& filePath);


	std::shared_ptr<MeshObject> m_yzPlane = nullptr;
	std::shared_ptr<MeshObject> m_xzPlane = nullptr;
	std::shared_ptr<MeshObject> m_xyPlane = nullptr;


	std::vector<std::vector<float>> m_vertWeights; // [i][j] represents the weight of cage vert j on model vert i
	//std::vector<std::vector<float>> m_normalWeights; // [i][j] represents the weight of cage face normal j on model vert i (only used for GC)

	void computeCageWeights();
	void deformModel();


	CoordinateTypes m_coordinateType = CoordinateTypes::MVC; // default is MVC


	void generateCage2();
	std::vector<glm::vec3> generatePointSetP2(MeshObject &out_obb, MeshObject &out_pointSetP);

	float m_voxelSize = 0.0f;
	glm::vec3 m_eigenV1 = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_eigenV2 = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_eigenV3 = glm::vec3(0.0f, 0.0f, 0.0f);
	float m_expandedMinScalarAlongV1 = 0.0f;
	float m_expandedMinScalarAlongV2 = 0.0f;
	float m_expandedMinScalarAlongV3 = 0.0f;

	std::vector<std::vector<std::vector<unsigned int>>> generateOBBSpace(std::vector<glm::vec3> const& pointSetP);
	MeshTree generateMeshTree(std::vector<std::vector<std::vector<unsigned int>>> const& obbSpace, unsigned int minV1Index, unsigned int maxV1Index, unsigned int minV2Index, unsigned int maxV2Index, unsigned int minV3Index, unsigned int maxV3Index, unsigned int const recursiveDepth);
	MeshTree terminateMeshTree(unsigned int const minV1Index, unsigned int const maxV1Index, unsigned int const minV2Index, unsigned int const maxV2Index, unsigned int const minV3Index, unsigned int const maxV3Index);

	unsigned int m_maxRecursiveDepth = 100;
	float m_eta = 1.1f; // for t1
	float m_zeta = 1.1f; // for t2


	//NOTE: RETURN -1 on no index found
	int searchForSpliceIndexOverV1(std::vector<std::vector<std::vector<unsigned int>>> const& obbSpace, unsigned int const minV1Index, unsigned int const maxV1Index, unsigned int const minV2Index, unsigned int const maxV2Index, unsigned int const minV3Index, unsigned int const maxV3Index, float const t2);
	int searchForSpliceIndexOverV2(std::vector<std::vector<std::vector<unsigned int>>> const& obbSpace, unsigned int const minV1Index, unsigned int const maxV1Index, unsigned int const minV2Index, unsigned int const maxV2Index, unsigned int const minV3Index, unsigned int const maxV3Index, float const t2);
	int searchForSpliceIndexOverV3(std::vector<std::vector<std::vector<unsigned int>>> const& obbSpace, unsigned int const minV1Index, unsigned int const maxV1Index, unsigned int const minV2Index, unsigned int const maxV2Index, unsigned int const minV3Index, unsigned int const maxV3Index, float const t2);

	// scalar for translating cage verts
	float m_deltaMove = 1.0f;
};
