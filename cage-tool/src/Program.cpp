#include "Program.h"

#include <Eigen/Dense>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/projection.hpp>

// STATICS (INIT)...
glm::vec3 const Program::s_CAGE_UNSELECTED_COLOUR = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 const Program::s_CAGE_SELECTED_COLOUR = glm::vec3(1.0f, 1.0f, 0.0f);
unsigned int const Program::s_MAX_RECURSIVE_DEPTH = 10;
//TODO: NOTE that current obb axes being used is initial OBB so the voxelization isnt that used

Program::Program() {

}

// Error callback for glfw errors
void Program::error(int error, char const* description) {
	std::cerr << description << std::endl;
}

// Called to start the program
void Program::start() {
	setupWindow();
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		std::cerr << glewGetErrorString(err) << std::endl;
	}

	/*
	bool err = gl3wInit() != 0;

	if (err)
	{
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
	}
	*/

	camera = std::make_shared<Camera>();
	renderEngine = std::make_shared<RenderEngine>(window, camera);
	InputHandler::setUp(renderEngine, camera);
	mainLoop();
}

// Creates GLFW window for the program and sets callbacks for input
void Program::setupWindow() {
	glfwSetErrorCallback(Program::error);
	if (0 == glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_SAMPLES, 16);
	window = glfwCreateWindow(1024, 1024, "CageTool", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // V-sync on

	glfwSetWindowUserPointer(window, this);

	glfwSetKeyCallback(window, InputHandler::key);
	glfwSetMouseButtonCallback(window, InputHandler::mouse);
	glfwSetCursorPosCallback(window, InputHandler::motion);
	glfwSetScrollCallback(window, InputHandler::scroll);
	glfwSetWindowSizeCallback(window, InputHandler::reshape);

	// Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

	char const* glsl_version = "#version 430 core";

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}


//NOTE: this method should only be called ONCE at start
void Program::initScene() {

	// SAFETY...
	meshObjects.clear();
	m_yzPlane = nullptr;
	m_xzPlane = nullptr;
	m_xyPlane = nullptr;
	m_model = nullptr;
	m_cage = nullptr;

	// CREATE THE 3 PLANES...

	// draw a symmetrical grid for each cartesian plane...

	//NOTE: compare this to far clipping plane distance of 2000
	//NOTE: all these should be the same
	int const maxX = 500;
	int const maxY = 500;
	int const maxZ = 500;
	//NOTE: any change here should be reflected in the ImGui notice
	int const deltaX = 10;
	int const deltaY = 10;
	int const deltaZ = 10;

	// YZ PLANE

	m_yzPlane = std::make_shared<MeshObject>();

	for (int y = -maxY; y <= maxY; y += deltaY) {
		m_yzPlane->drawVerts.push_back(glm::vec3(0.0f, y, -maxZ));
		m_yzPlane->drawVerts.push_back(glm::vec3(0.0f, y, maxZ));
	}
	for (int z = -maxZ; z <= maxZ; z += deltaZ) {
		m_yzPlane->drawVerts.push_back(glm::vec3(0.0f, -maxY, z));
		m_yzPlane->drawVerts.push_back(glm::vec3(0.0f, maxY, z));
	}

	for (unsigned int i = 0; i < m_yzPlane->drawVerts.size(); ++i) {
		m_yzPlane->drawFaces.push_back(i);
		m_yzPlane->colours.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	}

	m_yzPlane->m_primitiveMode = PrimitiveMode::LINES;
	meshObjects.push_back(m_yzPlane);
	renderEngine->assignBuffers(*m_yzPlane);

	// XZ PLANE

	m_xzPlane = std::make_shared<MeshObject>();

	for (int x = -maxX; x <= maxX; x += deltaX) {
		m_xzPlane->drawVerts.push_back(glm::vec3(x, 0.0f, -maxZ));
		m_xzPlane->drawVerts.push_back(glm::vec3(x, 0.0f, maxZ));
	}
	for (int z = -maxZ; z <= maxZ; z += deltaZ) {
		m_xzPlane->drawVerts.push_back(glm::vec3(-maxX, 0.0f, z));
		m_xzPlane->drawVerts.push_back(glm::vec3(maxX, 0.0f, z));
	}

	for (unsigned int i = 0; i < m_xzPlane->drawVerts.size(); ++i) {
		m_xzPlane->drawFaces.push_back(i);
		m_xzPlane->colours.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	}

	m_xzPlane->m_primitiveMode = PrimitiveMode::LINES;
	meshObjects.push_back(m_xzPlane);
	renderEngine->assignBuffers(*m_xzPlane);

	// XY PLANE

	m_xyPlane = std::make_shared<MeshObject>();

	for (int x = -maxX; x <= maxX; x += deltaX) {
		m_xyPlane->drawVerts.push_back(glm::vec3(x, -maxY, 0.0f));
		m_xyPlane->drawVerts.push_back(glm::vec3(x, maxY, 0.0f));
	}
	for (int y = -maxY; y <= maxY; y += deltaY) {
		m_xyPlane->drawVerts.push_back(glm::vec3(-maxX, y, 0.0f));
		m_xyPlane->drawVerts.push_back(glm::vec3(maxX, y, 0.0f));
	}

	for (unsigned int i = 0; i < m_xyPlane->drawVerts.size(); ++i) {
		m_xyPlane->drawFaces.push_back(i);
		m_xyPlane->colours.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
	}

	m_xyPlane->m_primitiveMode = PrimitiveMode::LINES;
	meshObjects.push_back(m_xyPlane);
	renderEngine->assignBuffers(*m_xyPlane);
}


void Program::clearModel() {
	if (nullptr == m_model) return;

	// find model in meshObjects and delete it...
	for (unsigned int i = 0; i < meshObjects.size(); ++i) {
		if (meshObjects.at(i) == m_model) {
			meshObjects.at(i) = nullptr;
			meshObjects.erase(meshObjects.begin() + i);
			break;
		}
	}

	m_model = nullptr;

	// vertWeights have now been invalidated, so clear them
	m_vertWeights.clear();
}


void Program::clearCage() {
	if (nullptr == m_cage) return;

	// find cage in meshObjects and delete it...
	for (unsigned int i = 0; i < meshObjects.size(); ++i) {
		if (meshObjects.at(i) == m_cage) {
			meshObjects.at(i) = nullptr;
			meshObjects.erase(meshObjects.begin() + i);
			break;
		}
	}

	m_cage = nullptr;

	// vertWeights have now been invalidated, so clear them
	m_vertWeights.clear();
}


void Program::loadModel(std::string const& filePath) {

	std::shared_ptr<MeshObject> newModel = ObjectLoader::createTriMeshObject(filePath, false, true); // force ignore normals (TODO: generate them ourselves)
	if (nullptr != newModel) {
		// remove any old model...
		clearModel();

		// init new model...
		m_model = newModel;
		if (m_model->hasTexture) m_model->textureID = renderEngine->loadTexture("textures/default.png"); // apply default texture (if there are uvs)
		m_model->generateNormals();
		//m_model->setScale(glm::vec3(0.02f, 0.02f, 0.02f));
		meshObjects.push_back(m_model);
		renderEngine->assignBuffers(*m_model);
	}
}


void Program::loadCage(std::string const& filePath) {

	std::shared_ptr<MeshObject> newCage = ObjectLoader::createTriMeshObject(filePath, true, true); // force ignore both uvs and normals if present in file
	if (nullptr != newCage) {
		// remove any old cage...
		clearCage();

		// init new cage...
		m_cage = newCage;

		// set cage black and also set picking colours...
		for (unsigned int i = 0; i < m_cage->colours.size(); ++i) {
			// render colour...
			m_cage->colours.at(i) = s_CAGE_UNSELECTED_COLOUR;

			// reference: http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
			// reference: https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_slow_easy.cpp
			// picking colour...
			//NOTE: this assumes that this cage will be the only object with picking colours (these colours must be unique)
			unsigned int const r = (i & 0x000000FF) >> 0;
			unsigned int const g = (i & 0x0000FF00) >> 8;
			unsigned int const b = (i & 0x00FF0000) >> 16;
			m_cage->pickingColours.push_back(glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f));
		}
		m_cage->m_polygonMode = PolygonMode::LINE; // set wireframe
		m_cage->m_renderPoints = true; // hack to render the cage as points as well (2nd polygon mode)

		//m_cage->setScale(glm::vec3(0.02f, 0.02f, 0.02f));
		meshObjects.push_back(m_cage);
		renderEngine->assignBuffers(*m_cage);
	}
}


void Program::drawUI() {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		ImGui::Begin("EDIT WINDOW");

		ImGui::Separator();

		ImGui::ColorEdit3("CLEAR COLOR", (float*)&clearColor);

		ImGui::Separator();

		if (nullptr != m_yzPlane) {
			if (ImGui::Button("TOGGLE YZ-PLANE (RED)")) m_yzPlane->m_isVisible = !m_yzPlane->m_isVisible;
			ImGui::SameLine();
		}
		if (nullptr != m_xzPlane) {
			if (ImGui::Button("TOGGLE XZ-PLANE (GREEN)")) m_xzPlane->m_isVisible = !m_xzPlane->m_isVisible;
			ImGui::SameLine();
		}
		if (nullptr != m_yzPlane) {
			if (ImGui::Button("TOGGLE XY-PLANE (BLUE)")) m_xyPlane->m_isVisible = !m_xyPlane->m_isVisible;
			ImGui::SameLine();
		}
		if (nullptr != m_yzPlane || nullptr != m_xzPlane || nullptr != m_yzPlane) {
			ImGui::Text("	note: grid spacing is 10 units");
			ImGui::Separator();
		}

		//TODO: optimize this in the future (e.g. only update position/rotation/scale if a field changed)
		//NOTE: for some reason (maybe by design), the IMGUI label names must be unique otherwise editing 1 also changes any others with same label
		//NOTE: follow-up to the above note: the label is hashed into an ID, thus same label equals same ID and thus same manipulation (a fix for this is to append ##<num> to make the hash different, but same visible label)
		if (nullptr != m_model) {
			ImGui::Text("MODEL");


			ImGui::PushItemWidth(200.0f);


			ImGui::Text("POSITION");
			float xPos = m_model->getPosition().x;
			ImGui::InputFloat("x##0", &xPos);
			m_model->setPosition(glm::vec3(xPos, m_model->getPosition().y, m_model->getPosition().z));

			ImGui::SameLine();
			float yPos = m_model->getPosition().y;
			ImGui::InputFloat("y##0", &yPos);
			m_model->setPosition(glm::vec3(m_model->getPosition().x, yPos, m_model->getPosition().z));

			ImGui::SameLine();
			float zPos = m_model->getPosition().z;
			ImGui::InputFloat("z##0", &zPos);
			m_model->setPosition(glm::vec3(m_model->getPosition().x, m_model->getPosition().y, zPos));


			ImGui::Text("ROTATION (degrees) - Z then Y then X (XYZ)");
			float xRot = m_model->getRotation().x;
			ImGui::InputFloat("x##1", &xRot);
			m_model->setRotation(glm::vec3(xRot, m_model->getRotation().y, m_model->getRotation().z));

			ImGui::SameLine();
			float yRot = m_model->getRotation().y;
			ImGui::InputFloat("y##1", &yRot);
			m_model->setRotation(glm::vec3(m_model->getRotation().x, yRot, m_model->getRotation().z));

			ImGui::SameLine();
			float zRot = m_model->getRotation().z;
			ImGui::InputFloat("z##1", &zRot);
			m_model->setRotation(glm::vec3(m_model->getRotation().x, m_model->getRotation().y, zRot));


			ImGui::Text("SCALE");
			float xScale = m_model->getScale().x;
			ImGui::InputFloat("x##2", &xScale);
			m_model->setScale(glm::vec3(xScale, m_model->getScale().y, m_model->getScale().z));

			ImGui::SameLine();
			float yScale = m_model->getScale().y;
			ImGui::InputFloat("y##2", &yScale);
			m_model->setScale(glm::vec3(m_model->getScale().x, yScale, m_model->getScale().z));

			ImGui::SameLine();
			float zScale = m_model->getScale().z;
			ImGui::InputFloat("z##2", &zScale);
			m_model->setScale(glm::vec3(m_model->getScale().x, m_model->getScale().y, zScale));


			ImGui::Separator();

			ImGui::PopItemWidth();
		}

		//NOTE: might not even have this for cage since it will have to surround the model anyway
		if (nullptr != m_cage) {
			ImGui::Text("CAGE");

			if (ImGui::Button("SELECT ALL VERTS")) selectCageVerts(0, m_cage->colours.size());
			ImGui::SameLine();
			if (ImGui::Button("UNSELECT ALL VERTS")) unselectCageVerts(0, m_cage->colours.size());
			ImGui::SameLine();
			if (ImGui::Button("TOGGLE ALL VERTS")) toggleCageVerts(0, m_cage->colours.size());

			ImGui::PushItemWidth(200.0f);


			ImGui::Text("POSITION");
			float xPos = m_cage->getPosition().x;
			ImGui::InputFloat("x##3", &xPos);
			m_cage->setPosition(glm::vec3(xPos, m_cage->getPosition().y, m_cage->getPosition().z));

			ImGui::SameLine();
			float yPos = m_cage->getPosition().y;
			ImGui::InputFloat("y##3", &yPos);
			m_cage->setPosition(glm::vec3(m_cage->getPosition().x, yPos, m_cage->getPosition().z));

			ImGui::SameLine();
			float zPos = m_cage->getPosition().z;
			ImGui::InputFloat("z##3", &zPos);
			m_cage->setPosition(glm::vec3(m_cage->getPosition().x, m_cage->getPosition().y, zPos));


			ImGui::Text("ROTATION (degrees) - Z then Y then X (XYZ)");
			float xRot = m_cage->getRotation().x;
			ImGui::InputFloat("x##4", &xRot);
			m_cage->setRotation(glm::vec3(xRot, m_cage->getRotation().y, m_cage->getRotation().z));

			ImGui::SameLine();
			float yRot = m_cage->getRotation().y;
			ImGui::InputFloat("y##4", &yRot);
			m_cage->setRotation(glm::vec3(m_cage->getRotation().x, yRot, m_cage->getRotation().z));

			ImGui::SameLine();
			float zRot = m_cage->getRotation().z;
			ImGui::InputFloat("z##4", &zRot);
			m_cage->setRotation(glm::vec3(m_cage->getRotation().x, m_cage->getRotation().y, zRot));


			ImGui::Text("SCALE");
			float xScale = m_cage->getScale().x;
			ImGui::InputFloat("x##5", &xScale);
			m_cage->setScale(glm::vec3(xScale, m_cage->getScale().y, m_cage->getScale().z));

			ImGui::SameLine();
			float yScale = m_cage->getScale().y;
			ImGui::InputFloat("y##5", &yScale);
			m_cage->setScale(glm::vec3(m_cage->getScale().x, yScale, m_cage->getScale().z));

			ImGui::SameLine();
			float zScale = m_cage->getScale().z;
			ImGui::InputFloat("z##5", &zScale);
			m_cage->setScale(glm::vec3(m_cage->getScale().x, m_cage->getScale().y, zScale));


			ImGui::Separator();

			ImGui::PopItemWidth();
		}

		if (nullptr != m_model && nullptr != m_cage) {
			if (m_vertWeights.empty()) {
				if (ImGui::Button("COMPUTE CAGE WEIGHTS")) computeCageWeights();
			}
			else {
				if (ImGui::Button("CLEAR CAGE WEIGHTS")) m_vertWeights.clear();
			}
			ImGui::Separator();
		}

		if (nullptr != m_model) {
			if (ImGui::Button("CLEAR MODEL")) clearModel();
		} else {
			//NOTE: it seems that imgui only allows typing in the text box upto maxFileNameLength - 1 chars.
			unsigned int const maxFileNameLength = 256;
			char filename[maxFileNameLength] = "";
			ImGuiInputTextFlags const flags = ImGuiInputTextFlags_EnterReturnsTrue;
			ImGui::Text("LOAD MODEL");
			ImGui::Text(".../models/imports/");
			ImGui::SameLine();
			if (ImGui::InputText(".obj##0", filename, IM_ARRAYSIZE(filename), flags)) {
				loadModel("models/imports/" + std::string(filename) + ".obj");
			}
		}
		ImGui::Separator();

		if (nullptr != m_cage) {
			if (ImGui::Button("CLEAR CAGE")) clearCage();
		} else {
			if (nullptr != m_model) {
				if (ImGui::Button("GENERATE CAGE")) generateCage2();
			}
			
			//NOTE: it seems that imgui only allows typing in the text box upto maxFileNameLength - 1 chars.
			unsigned int const maxFileNameLength = 256;
			char filename[maxFileNameLength] = "";
			ImGuiInputTextFlags const flags = ImGuiInputTextFlags_EnterReturnsTrue;
			ImGui::Text("LOAD CAGE");
			ImGui::Text(".../models/imports/");
			ImGui::SameLine();
			if (ImGui::InputText(".obj##1", filename, IM_ARRAYSIZE(filename), flags)) {
				loadCage("models/imports/" + std::string(filename) + ".obj");
			}
		}
		ImGui::Separator();

		ImGui::End();
	}

}

// Main loop
void Program::mainLoop() {

	initScene();

	// Our state
	clearColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); //NOTE: keep this white since it has the least chance of interfering with color picking (due to being the last possible color that can be generated)

	while(!glfwWindowShouldClose(window)) {

		//NOTE: any colour picking will be done in mouse callback...
		glfwPollEvents();

		drawUI();

		// Rendering
		ImGui::Render();
		glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
		glClear(GL_COLOR_BUFFER_BIT);

		renderEngine->render(meshObjects);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Clean up, program needs to exit
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	window = nullptr;
	glfwTerminate();
}


//REFERENCES:
// https://www.cse.wustl.edu/~taoju/research/meanvalue.pdf
// I followed the "Robust" algorithm outlined in the above paper
//TODO: check if more safety cases need to be added for protecting from divide by 0 and resulting -nan
//TODO: safety checking might have to be added, or otherwise make sure the calculation still works if cage face is less than 1 unit from model (does the unit sphere need to be shrunk more?)
//TODO: implement HC/GC weights
// this method computes the vector of cage vertex weights for every model vertex, in the current orientation of both meshes
//TODO: OPTIMIZE THIS BY CACHING SOME OF THE REUSED CALCULATIONS
void Program::computeCageWeights() {
	
	// cleanup...
	m_vertWeights.clear();
	//m_normalWeights.clear();
	
	if (nullptr == m_model || nullptr == m_cage) return;

	// 0. init vector sizes...

	m_vertWeights = std::vector<std::vector<float>>(m_model->drawVerts.size(), std::vector<float>(m_cage->drawVerts.size(), 0.0f));
	//TODO: init m_normalWeights

	// compute new weights based on set coord type...

	if (CoordinateTypes::MVC == m_coordinateType) {

		// compute vertWeights...

		// foreach model vert...
		for (unsigned int i = 0; i < m_model->drawVerts.size(); ++i) {

			// init the cage vert weights vector for model vert i...
			std::vector<float> u_i(m_cage->drawVerts.size(), 0.0f);

			glm::vec3 const x = m_model->drawVerts.at(i); // (sphere origin)

			// foreach triangle face in cage mesh...
			for (unsigned int f = 0; f < m_cage->drawFaces.size(); f += 3) {

				// save the 3 cage vert indices making up cage face f...
				unsigned int const p1_index = m_cage->drawFaces.at(f);
				unsigned int const p2_index = m_cage->drawFaces.at(f+1);
				unsigned int const p3_index = m_cage->drawFaces.at(f+2);

				// get the 3 cage vert positions...
				glm::vec3 const p1 = m_cage->drawVerts.at(p1_index);
				glm::vec3 const p2 = m_cage->drawVerts.at(p2_index);
				glm::vec3 const p3 = m_cage->drawVerts.at(p3_index);

				float const d1 = glm::length(p1 - x);
				float const d2 = glm::length(p2 - x);
				float const d3 = glm::length(p3 - x);

				//TODO: ...
				//TODO: ... (i'm thinking give it a weight of 1 and enter a special mode (if mode is false, then set true) - since this cage vert is basically located at model vert, it should get highest influence
				// PREVENTS DIVIDE BY ZERO (since we would be trying to normalize the zero vector which is undefined)
				if (d1 < glm::epsilon<float>()) {
					//return? or continue? or something else?
					return;
				}
				if (d2 < glm::epsilon<float>()) {
					//return? or continue? or something else?
					return;
				}
				if (d3 < glm::epsilon<float>()) {
					//return? or continue? or something else?
					return;
				}

				glm::vec3 const u1 = (p1 - x) / d1; // p1 projected on unit sphere centered at x
				glm::vec3 const u2 = (p2 - x) / d2; // p2 projected on unit sphere centered at x
				glm::vec3 const u3 = (p3 - x) / d3; // p3 projected on unit sphere centered at x

				// side-lengths of planar triangle t
				//NOTE: I believe the length would vary from 0 to 2 (e.g. 2 points on opposite side of unit sphere = R+R = 1+1 = 2)
				float const l1 = glm::length(u2 - u3);
				float const l2 = glm::length(u3 - u1);
				float const l3 = glm::length(u1 - u2);

				// arc-lengths of spherical triangle t_sph (equivalent to angles since R=1)
				//NOTE: I believe these angles/lengths will be between 0 and pi
				float const theta1 = 2 * glm::asin(l1 / 2);
				float const theta2 = 2 * glm::asin(l2 / 2);
				float const theta3 = 2 * glm::asin(l3 / 2);

				// safety (handle if a length happens to be very small (< 2*epsilon) (I guess the area would be 0 and thus influence by this face would be 0, thus just continue?))
				// PREVENTS DIVIDE BY ZERO (since asin(0/2) = 0, thus theta = 0, which then cause a sin(0) in denominator for a c_i calculation
				if (theta1 < glm::epsilon<float>() || theta2 < glm::epsilon<float>() || theta3 < glm::epsilon<float>()) continue;

				// half-angle (Beyer)
				//NOTE: assume a scenario where u1,u2,u3 form a planar triangle that cuts through the center (x) of the unit sphere (and recall that they are all on the surface of the sphere at R=1).
				//NOTE: now assume we have a configuration like
				// u1-R-x-R-u3
				//   \  |  /
				//    \ R /
				//     \|/
				//     u2
				// the planar triangle lengths would be l1 = l3 = sqrt(2*R^2) = sqrt(2) and l2 = 2*R = 2
				// thus, we would get theta1 + theta2 + theta3 = 2 * [asin(sqrt(2) / 2) + asin(1) + asin(sqrt(2) / 2)] = 2 * [pi/4 + pi/2 + pi/4] = 2 * [pi]
				//NOTE: thus, I think h will be in range [1.5 * epsilon, pi]
				float const h = (theta1 + theta2 + theta3) / 2;
				if (glm::pi<float>() - h < glm::epsilon<float>()) {
					// center of sphere point x lies on triangle t (use 2D barycentric coords)

					std::fill(u_i.begin(), u_i.end(), 0.0f); // reset weights vector back to all zeros

					//NOTE: only this face will have an influence on model vert_i (x)
					u_i.at(p1_index) = glm::sin(theta1) * d3 * d2;
					u_i.at(p2_index) = glm::sin(theta2) * d1 * d3;
					u_i.at(p3_index) = glm::sin(theta3) * d2 * d1;
					break; // no need to check any other faces
				}

				//NOTE: I was having a bug where some faces were missing due to one of these cosines (c1,c2,c3) being something like 1.000024 which squared is > 1 and thus causes a sqrt(<0) at one of s1,s2,s3 resulting in a -nan
				//FIX: clamp the cosines between the mathmetical range of -1 to 1
				//NOTE: I'm not sure if clamping is the right thing to do, or if it should be an error or something, but I think its just float imprecision causing it and the program seems to work...
				
				// cosines of the spherical triangle angles (diheral angles)
				float const c1 = glm::clamp((2 * glm::sin(h)*glm::sin(h - theta1)) / (glm::sin(theta2)*glm::sin(theta3)) - 1, -1.0f, 1.0f);
				float const c2 = glm::clamp((2 * glm::sin(h)*glm::sin(h - theta2)) / (glm::sin(theta3)*glm::sin(theta1)) - 1, -1.0f, 1.0f);
				float const c3 = glm::clamp((2 * glm::sin(h)*glm::sin(h - theta3)) / (glm::sin(theta1)*glm::sin(theta2)) - 1, -1.0f, 1.0f);

				//TODO: add any error checking or comments for below???
				glm::mat3 const uMat = glm::mat3(u1, u2, u3);
				float const det = glm::determinant(uMat);
				
				float const s1 = glm::sign(det) * glm::sqrt(1 - c1 * c1);
				float const s2 = glm::sign(det) * glm::sqrt(1 - c2 * c2);
				float const s3 = glm::sign(det) * glm::sqrt(1 - c3 * c3);

				// if sphere origin (x) lies on same plane as triangle t, but lies outside triangle, then we ignore this face (since projection would be a curve of zero area and thus have 0 weight)
				if (glm::abs(s1) <= glm::epsilon<float>() || glm::abs(s2) <= glm::epsilon<float>() || glm::abs(s3) <= glm::epsilon<float>()) continue; // continue to next face

				// update the weights of each of the 3 cage verts making up this face affecting the model vert_i (x) by accumulation
				u_i.at(p1_index) += (theta1 - c2 * theta3 - c3 * theta2) / (d1 * glm::sin(theta2) * s3);
				u_i.at(p2_index) += (theta2 - c3 * theta1 - c1 * theta3) / (d2 * glm::sin(theta3) * s1);
				u_i.at(p3_index) += (theta3 - c1 * theta2 - c2 * theta1) / (d3 * glm::sin(theta1) * s2);
			}

			// 6. normalize the weights vector (sum of all elements = 1) for affine property

			//TODO: since, we can have negative weights, isn't it possible that totalW could be 0?

			float totalW = 0.0f;
			for (unsigned int j = 0; j < u_i.size(); ++j) {
				totalW += u_i.at(j);
			}

			for (unsigned int j = 0; j < u_i.size(); ++j) {
				u_i.at(j) /= totalW;
			}

			// 7. assign weights vector to the model vert i
			m_vertWeights.at(i) = u_i; // insert finished weight vector into matrix
		}

	} else if (CoordinateTypes::HC == m_coordinateType) {
		//TODO (compute vertWeights) - low priority
	} else if (CoordinateTypes::GC == m_coordinateType) {
		//TODO (compute vertWeights) - low priority
		//TODO (compute normalWeights) - low priority
	} else {
		std::cout << "ERROR (Program.cpp) - INVALID COORDINATE TYPE" << std::endl;
		return;
	}

}


//TODO: testing
void Program::deformModel() {

	// if one (or both) objects has not been loaded in, we cannot apply algorithm
	if (nullptr == m_model || nullptr == m_cage) return;

	// if we have no weights (e.g. user hasn't pressed compute cage weights button yet or they have cleared the weights), we cannot apply algorithm
	if (m_vertWeights.size() == 0) return;

	// NOTATION (following course notes)...
	std::vector<glm::vec3> const& v = m_cage->drawVerts;
	std::vector<std::vector<float>> const& u = m_vertWeights; // size (n+1)x(m+1)

	//std::vector<glm::vec3> const& psi = m_cage->faceNormals; //TODO: implement later
	//std::vector<std::vector<float>> const& omega = m_normalWeights; // size (n+1)x(k+1)

	unsigned int const n = m_model->drawVerts.size() - 1;
	unsigned int const m = v.size() - 1;
	//unsigned int const k = psi.size() - 1;

	// foreach model vert...
	for (unsigned int i = 0; i <= n; ++i) {
		// update model vert i (c_i) as a linear combo of cage verts (MVC, HC, GC) + cage face normals (GC only)
		glm::vec3 c_i = glm::vec3(0.0f, 0.0f, 0.0f);

		std::vector<float> const& u_i = u.at(i);

		// for each cage vert...
		for (unsigned int j = 0; j <= m; ++j) {
			c_i += u_i.at(j) * v.at(j);
		}

		/*
		if (CoordinateTypes::GC == m_coordinateType) {

			std::vector<float> const& omega_i = omega.at(i);

			// for each cage face normal...
			for (unsigned int j = 0; j <= k; ++j) {
				c_i += omega_i.at(j) * psi.at(j);
			}
		}
		*/

		m_model->drawVerts.at(i) = c_i; // update
	}

	// recompute the model's normals now that its verts have changed... 
	m_model->generateNormals();
	renderEngine->updateBuffers(*m_model, true, false, true, false);
}


//NOTE: both startIndex and endIndex will be inclusive
void Program::selectCageVerts(unsigned int const startIndex, unsigned int const count) {
	// error handling...
	if (nullptr == m_cage || startIndex >= m_cage->colours.size() || count == 0) return;

	// clamp endIndex max bound to last index in colours vector
	unsigned int const endIndex = (startIndex + count - 1) >= m_cage->colours.size() ? m_cage->colours.size() - 1 : startIndex + count - 1;

	// set all specified verts to selected colour...
	for (unsigned int i = startIndex; i <= endIndex; ++i) {
		m_cage->colours.at(i) = s_CAGE_SELECTED_COLOUR;
	}

	// update colour buffer
	// this enum test should always evaluate to true, but in case it isn't we can skip the update buffers, since the picking colours would just be reinserted (unchanged)
	if (ColourMode::NORMAL == m_cage->m_colourMode) renderEngine->updateBuffers(*m_cage, false, false, false, true);
}


//NOTE: both startIndex and endIndex will be inclusive
void Program::unselectCageVerts(unsigned int const startIndex, unsigned int const count) {
	// error handling...
	if (nullptr == m_cage || startIndex >= m_cage->colours.size() || count == 0) return;

	// clamp endIndex max bound to last index in colours vector
	unsigned int const endIndex = (startIndex + count - 1) >= m_cage->colours.size() ? m_cage->colours.size() - 1 : startIndex + count - 1;

	// set all specified verts to unselected colour...
	for (unsigned int i = startIndex; i <= endIndex; ++i) {
		m_cage->colours.at(i) = s_CAGE_UNSELECTED_COLOUR;
	}

	// update colour buffer
	// this enum test should always evaluate to true, but in case it isn't we can skip the update buffers, since the picking colours would just be reinserted (unchanged)
	if (ColourMode::NORMAL == m_cage->m_colourMode) renderEngine->updateBuffers(*m_cage, false, false, false, true);
}


//NOTE: both startIndex and endIndex will be inclusive
void Program::toggleCageVerts(unsigned int const startIndex, unsigned int const count) {
	// error handling...
	if (nullptr == m_cage || startIndex >= m_cage->colours.size() || count == 0) return;

	// clamp endIndex max bound to last index in colours vector
	unsigned int const endIndex = (startIndex + count - 1) >= m_cage->colours.size() ? m_cage->colours.size() - 1 : startIndex + count - 1;

	// set all specified verts to opposite colour...
	for (unsigned int i = startIndex; i <= endIndex; ++i) {
		m_cage->colours.at(i) = s_CAGE_UNSELECTED_COLOUR == m_cage->colours.at(i) ? s_CAGE_SELECTED_COLOUR : s_CAGE_UNSELECTED_COLOUR;
	}

	// update colour buffer
	// this enum test should always evaluate to true, but in case it isn't we can skip the update buffers, since the picking colours would just be reinserted (unchanged)
	if (ColourMode::NORMAL == m_cage->m_colourMode) renderEngine->updateBuffers(*m_cage, false, false, false, true);
}

void Program::translateSelectedCageVerts(glm::vec3 const& translation) {
	if (nullptr == m_cage) return;

	bool hasChanged = false;

	// loop through all cage verts...
	for (unsigned int i = 0; i < m_cage->drawVerts.size(); ++i) {
		glm::vec3 &vert = m_cage->drawVerts.at(i);

		// if vert is selected, apply translation to it
		if (s_CAGE_SELECTED_COLOUR == m_cage->colours.at(i)) {
			vert += translation;
			hasChanged = true;
		}
	}

	//TODO: add in call to recompute normals of cage (if we are including normals with cage) - would need to set updateNormals true in updateBuffers() call

	if (hasChanged) {
		renderEngine->updateBuffers(*m_cage, true, false, false, false);
	
		//TODO: add in call to deformModel() if its not null
		deformModel();
	}
}



// SIMILAR TO IMPROVED OBB METHOD (XIAN, LIN, GAO)...
// reference: http://www.cad.zju.edu.cn/home/hwlin/pdf_files/Automatic-cage-generation-by-improved-OBBs-for-mesh-deformation.pdf
// reference: http://www-home.htwg-konstanz.de/~umlauf/Papers/cagesurvSinCom.pdf
// reference: https://hewjunwei.wordpress.com/2013/01/26/obb-generation-via-principal-component-analysis/
void Program::generateCage() {
	if (nullptr == m_model) return;

	std::vector<glm::vec3> pointSetP = generatePointSetP();

	std::shared_ptr<MeshObject> obbTree = generateOBBs(pointSetP, 0);

	//TODO: construct cage out of obbTree for rendering (e.g. add colours/picking colours, flags, etc.)

	// clear old cage if any...
	clearCage();

	m_cage = obbTree;

	// init vert colours (uniform light grey for now)
	for (unsigned int i = 0; i < m_cage->drawVerts.size(); ++i) {
		m_cage->colours.push_back(glm::vec3(0.8f, 0.8f, 0.8f));
	}

	// set cage black and also set picking colours...
	for (unsigned int i = 0; i < m_cage->colours.size(); ++i) {
		// render colour...
		m_cage->colours.at(i) = s_CAGE_UNSELECTED_COLOUR;

		// reference: http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
		// reference: https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_slow_easy.cpp
		// picking colour...
		//NOTE: this assumes that this cage will be the only object with picking colours (these colours must be unique)
		unsigned int const r = (i & 0x000000FF) >> 0;
		unsigned int const g = (i & 0x0000FF00) >> 8;
		unsigned int const b = (i & 0x00FF0000) >> 16;
		m_cage->pickingColours.push_back(glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f));
	}
	m_cage->m_polygonMode = PolygonMode::LINE; // set wireframe
	//m_cage->m_renderPoints = true; // hack to render the cage as points as well (2nd polygon mode)

	//m_cage->setScale(glm::vec3(0.02f, 0.02f, 0.02f));
	meshObjects.push_back(m_cage);
	renderEngine->assignBuffers(*m_cage);
}

std::vector<glm::vec3> Program::generatePointSetP() {
	if (nullptr == m_model) return std::vector<glm::vec3>();

	// 1. extract set of model verts (eliminate duplicates)...
	std::vector<glm::vec3> pointSetM;
	for (glm::vec3 const& p : m_model->drawVerts) {
		auto it = std::find(pointSetM.begin(), pointSetM.end(), p);
		if (pointSetM.end() == it) { // new unique point
			pointSetM.push_back(p);
		}
	}

	// 2. generate initial OBB O of pointSetM using Principal Component Analysis...

	// low covariance of 2 values means more independent (less correlated)
	// covariance(x,x) = variance(x)
	// covariance matrix A generalizes the concept of variance in multiple dimensions
	// we want to diagonalize the covariance matrix to make signals strong (big diagonal entries) and distortions weak (low (in this case 0) off-diagonal entries)
	// A is REAL and SYMMETRIC ===> guaranteed to have 3 eigenvectors in the similarity transformation (change of basis matrix) when we diagonalize A.
	// THE 3 EIGENVECTORS OF COVARIANCE MATRIX WILL MAKE UP ORIENTATION OF OBB
	// large eigenvalues mean large variance, thus align OBB along eigenvector corresponding to largest eigenvalue

	//TODO: could improve this algo by working on the convex hull points only (but it might not be worth it)
	//TODO: could improve this program by using a more exact OBB method (slower) for low vert count models.

	// reference: https://stackoverflow.com/questions/6189229/creating-oobb-from-points
	// 3. compute centroid mu of points...
	glm::vec3 mu = glm::vec3(0.0f, 0.0f, 0.0f);
	for (glm::vec3 const& p : pointSetM) {
		mu += p;
	}
	mu /= pointSetM.size();

	// 4. compute covariance matrix...
	glm::mat3 covarianceMatrix = glm::mat3(0.0f);
	for (glm::vec3 const& p : pointSetM) {
		glm::vec3 const dev = p - mu;
		glm::mat3 const covMat_i = glm::outerProduct(dev, dev); // dev * (dev)^T
		covarianceMatrix += covMat_i;
	}
	//NOTE: this matrix should be REAL and SYMMETRIC

	// 5. get sorted eigenvalues (ascending) + corresponding eigenvectors...

	// convert matrix form (glm -> eigen) to work with eigensolver...
	Eigen::Matrix3f covMatEigen;
	for (unsigned int i = 0; i < covarianceMatrix.length(); ++i) { // loop over columns
		for (unsigned int j = 0; j < covarianceMatrix[i].length(); ++j) { // loop over rows
			covMatEigen(j, i) = covarianceMatrix[i][j]; //NOTE: eigen(row, col) vs glm(col, row)
		}
	}

	// reference: https://stackoverflow.com/questions/50458712/c-find-eigenvalues-and-eigenvectors-of-matrix
	Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigenSolver;
	eigenSolver.compute(covMatEigen);
	Eigen::Vector3f eigenValues = eigenSolver.eigenvalues(); //NOTE: eigenvalues come sorted in ascending order
	Eigen::Matrix3f eigenVectors = eigenSolver.eigenvectors(); //NOTE: eigenvectors come sorted corresponding to returned eigenvalues and are normalized. This returned matrix also corresponds to P in the diagonalization formula A = P*D*P^-1. Here A =:= covariance matrix

	// convert eigenvector forms (eigen -> glm)...
	glm::vec3 eigenV1 = glm::vec3(eigenVectors(0, 0), eigenVectors(1, 0), eigenVectors(2, 0));
	glm::vec3 eigenV2 = glm::vec3(eigenVectors(0, 1), eigenVectors(1, 1), eigenVectors(2, 1));
	glm::vec3 eigenV3 = glm::vec3(eigenVectors(0, 2), eigenVectors(1, 2), eigenVectors(2, 2));

	//TODO: SORT THESE!!!
	//m_eigenV1 = eigenV1;
	//m_eigenV2 = eigenV2;
	//m_eigenV3 = eigenV3;

	// find the 8 bounding vertices by finding the min/max coordinates of the projected points along each of the 3 mutually orthonormal eigenvectors that form our basis...
	float minScalarAlongV1 = std::numeric_limits<float>::max();
	float maxScalarAlongV1 = std::numeric_limits<float>::lowest();
	float minScalarAlongV2 = std::numeric_limits<float>::max();
	float maxScalarAlongV2 = std::numeric_limits<float>::lowest();
	float minScalarAlongV3 = std::numeric_limits<float>::max();
	float maxScalarAlongV3 = std::numeric_limits<float>::lowest();
	for (glm::vec3 const& p : pointSetM) {
		// project along eigenV1 axis...
		float const projScalarV1 = glm::dot(p, eigenV1) / glm::length2(eigenV1);
		if (projScalarV1 < minScalarAlongV1) minScalarAlongV1 = projScalarV1;
		if (projScalarV1 > maxScalarAlongV1) maxScalarAlongV1 = projScalarV1;

		// project along eigenV2 axis...
		float const projScalarV2 = glm::dot(p, eigenV2) / glm::length2(eigenV2);
		if (projScalarV2 < minScalarAlongV2) minScalarAlongV2 = projScalarV2;
		if (projScalarV2 > maxScalarAlongV2) maxScalarAlongV2 = projScalarV2;

		// project along eigenV3 axis...
		float const projScalarV3 = glm::dot(p, eigenV3) / glm::length2(eigenV3);
		if (projScalarV3 < minScalarAlongV3) minScalarAlongV3 = projScalarV3;
		if (projScalarV3 > maxScalarAlongV3) maxScalarAlongV3 = projScalarV3;
	}

	// 6. compute a cubic voxel size (side length)...
	//TODO: make sure this is never 0. could clamp lower bound as epsilon or use something like 0.001
	//TESTING THIS...
	float const avgExtent = ((maxScalarAlongV1 - minScalarAlongV1) + (maxScalarAlongV2 - minScalarAlongV2) + (maxScalarAlongV3 - minScalarAlongV3)) / 3;
	float const voxelSize = avgExtent / 100; // NOTE: increase denominator in order to increase voxel resolution (voxel count) - NOTE: that scaling the denom causes a cubic scale to the voxel count (so RAM explodes pretty quickly)
	m_voxelSize = voxelSize;

	// 7. voxelize our OBB into a slightly larger (or if lucky same size) 3D grid of cubes...
	// this expanded voxelized OBB will have new min/max scalars along each of the 3 basis axes denoting boundaries
	//NOTE: both the OBB and expanded voxelized-OBB will still have same centroid

	float const midScalarAlongV1 = (minScalarAlongV1 + maxScalarAlongV1) / 2;
	float const midScalarAlongV2 = (minScalarAlongV2 + maxScalarAlongV2) / 2;
	float const midScalarAlongV3 = (minScalarAlongV3 + maxScalarAlongV3) / 2;

	unsigned int const voxelCountAlongHalfV1 = glm::ceil((maxScalarAlongV1 - midScalarAlongV1) / voxelSize);
	unsigned int const voxelCountAlongHalfV2 = glm::ceil((maxScalarAlongV2 - midScalarAlongV2) / voxelSize);
	unsigned int const voxelCountAlongHalfV3 = glm::ceil((maxScalarAlongV3 - midScalarAlongV3) / voxelSize);

	float const expandedHalfExtentV1 = voxelCountAlongHalfV1 * voxelSize;
	float const expandedHalfExtentV2 = voxelCountAlongHalfV2 * voxelSize;
	float const expandedHalfExtentV3 = voxelCountAlongHalfV3 * voxelSize;

	float const expandedMinScalarAlongV1 = midScalarAlongV1 - expandedHalfExtentV1;
	float const expandedMaxScalarAlongV1 = midScalarAlongV1 + expandedHalfExtentV1;
	float const expandedMinScalarAlongV2 = midScalarAlongV2 - expandedHalfExtentV2;
	float const expandedMaxScalarAlongV2 = midScalarAlongV2 + expandedHalfExtentV2;
	float const expandedMinScalarAlongV3 = midScalarAlongV3 - expandedHalfExtentV3;
	float const expandedMaxScalarAlongV3 = midScalarAlongV3 + expandedHalfExtentV3;

	// init arrays...

	// coloured voxel array (BLACK =:= OUTER (default), CYAN =:= FEATURE, MAGENTA =:= INNER)
	// those colours can be assigned later if we want to render the voxels, but for now treat BLACK == 0, CYAN == 1, MAGENTA = 2

	unsigned int const OUTER_BLACK = 0;
	unsigned int const FEATURE_CYAN = 1;
	unsigned int const INNER_MAGENTA = 2;


	unsigned int const nV1 = 2 * voxelCountAlongHalfV1;
	unsigned int const nV2 = 2 * voxelCountAlongHalfV2;
	unsigned int const nV3 = 2 * voxelCountAlongHalfV3;

	// either 0, 1, or 2
	std::vector<std::vector<std::vector<unsigned int>>> voxelClasses(nV3, std::vector<std::vector<unsigned int>>(nV2, std::vector<unsigned int>(nV1, OUTER_BLACK)));
	//NOTE: below, this 3D array maps voxel [V3][V2][V1] with either glm::vec3(0.0f, 0.0f, 0.0f) or another non-zero vec3.
	//~~~~~ this vec3 will be non-zero if this voxel is found to be a FEATURE VOXEL (meaning it has intersected at least 1 triangle face of model).
	//~~~~~ if this feature voxel is found to intersect multiple triangle faces, then this vec3 is taken as the trivial average of the face normals (normalized sum of them)
	//TODO: there is a known issue with some voxels being wrongly marker INNER, this is likely due to the wrong normal being considered since we just take the average right now
	//~~~~~ this can be fixed/improved by instead using the face normal with intersection point with smallest projected scalar along eigenV1 (scan direction)
	std::vector<std::vector<std::vector<glm::vec3>>> voxelIntersectedFaceAvgNormals(nV3, std::vector<std::vector<glm::vec3>>(nV2, std::vector<glm::vec3>(nV1, glm::vec3(0.0f, 0.0f, 0.0f))));

	// 8. CLASSIFY FEATURE VOXELS...

	// per triangle...
	for (unsigned int f = 0; f < m_model->drawFaces.size(); f += 3) {
		// get 3 triangle verts in x,y,z coordinate frame...
		glm::vec3 const& tV1xyz = m_model->drawVerts.at(m_model->drawFaces.at(f));
		glm::vec3 const& tV2xyz = m_model->drawVerts.at(m_model->drawFaces.at(f + 1));
		glm::vec3 const& tV3xyz = m_model->drawVerts.at(m_model->drawFaces.at(f + 2));

		// project these 3 points into OBB frame (measured by scalars along each of the 3 basis eigenvectors)...
		// .x will be eigenV1 scalar, .y is V2, .z is V3
		glm::vec3 const tV1 = glm::vec3(glm::dot(tV1xyz, eigenV1) / glm::length2(eigenV1), glm::dot(tV1xyz, eigenV2) / glm::length2(eigenV2), glm::dot(tV1xyz, eigenV3) / glm::length2(eigenV3));
		glm::vec3 const tV2 = glm::vec3(glm::dot(tV2xyz, eigenV1) / glm::length2(eigenV1), glm::dot(tV2xyz, eigenV2) / glm::length2(eigenV2), glm::dot(tV2xyz, eigenV3) / glm::length2(eigenV3));
		glm::vec3 const tV3 = glm::vec3(glm::dot(tV3xyz, eigenV1) / glm::length2(eigenV1), glm::dot(tV3xyz, eigenV2) / glm::length2(eigenV2), glm::dot(tV3xyz, eigenV3) / glm::length2(eigenV3));

		// DISCRETIZE TRIANGLE FACE BY BARYCENTRIC COORDS...

		//NOTE: this will be susceptible to duplicates
		//TODO: make sure bounds actually are [0, 1] exactly (rather than stopping short before 1) - should be done now

		// NOTE: all these points will be in OBB space
		std::vector<glm::vec3> discreteTrianglePts;
		discreteTrianglePts.push_back(tV1);
		discreteTrianglePts.push_back(tV2);
		discreteTrianglePts.push_back(tV3);


		float const du = voxelSize / (glm::distance(tV1, glm::proj(tV1, tV3 - tV2)));
		float const dv = voxelSize / (glm::distance(tV2, glm::proj(tV2, tV3 - tV1)));

		for (float u = 0.0f; u <= 1.0f; u += glm::min<float>(du, 1.0f - u > 0.0f ? 1.0f - u : du)) {
			for (float v = 0.0f; v <= 1.0f - u; v += glm::min<float>(dv, 1.0f - u - v > 0.0f ? 1.0f - u - v : dv)) {
				float const w = 1.0f - u - v;
				discreteTrianglePts.push_back(u * tV1 + v * tV2 + w * tV3);
			}
		}

		// NOW FOR EVERY TRIANGLE POINT...
		// check voxel its in (by mapping formula) and mark FEATURE VOXEL, add to list of considered voxels for this tri-face and if not already considered add this face normal as contribution

		// inverse mapping formula (position to voxel index)
		// i on V3, j on V2, k on V1
		// storing already considered (by this face) feature voxels as vec3(i,j,k)

		std::vector<glm::vec3> consideredVoxels;
		for (glm::vec3 const& tPt : discreteTrianglePts) {
			// inverse map back to indices i,j,k (single voxel)...

			//TODO: need to handle top most voxels to also include ceil
			unsigned int const indexI = glm::floor((tPt.z - expandedMinScalarAlongV3) / voxelSize);
			unsigned int const indexJ = glm::floor((tPt.y - expandedMinScalarAlongV2) / voxelSize);
			unsigned int const indexK = glm::floor((tPt.x - expandedMinScalarAlongV1) / voxelSize);

			glm::vec3 const intersectedVoxel = glm::vec3(indexI, indexJ, indexK);
			auto it = std::find(consideredVoxels.begin(), consideredVoxels.end(), intersectedVoxel);
			if (consideredVoxels.end() == it) { // new considered voxel

				// mark voxel as FEATURE...
				voxelClasses.at(indexI).at(indexJ).at(indexK) = FEATURE_CYAN;

				// contribute this face's normal to voxel
				voxelIntersectedFaceAvgNormals.at(indexI).at(indexJ).at(indexK) += m_model->faceNormals.at(f / 3);

				// mark voxel as considered by this tri-face
				consideredVoxels.push_back(intersectedVoxel);
			}
		}
	}

	// 8.5. foreach voxel, normalize (avg) the contributed face normals

	// per voxel...
	for (unsigned int i = 0; i < nV3; ++i) {
		for (unsigned int j = 0; j < nV2; ++j) {
			for (unsigned int k = 0; k < nV1; ++k) {
				// if voxel was marked as FEATURE...
				if (FEATURE_CYAN == voxelClasses.at(i).at(j).at(k)) {
					//TODO: make sure the tri-face normals weren't symbolic 0 vector due to being really small???
					voxelIntersectedFaceAvgNormals.at(i).at(j).at(k) = glm::normalize(voxelIntersectedFaceAvgNormals.at(i).at(j).at(k));
				}
			}
		}
	}

	// 9. CLASSIFY INNER VOXELS (SCAN-ALGORITHM)...
	// reference: http://blog.wolfire.com/2009/11/Triangle-mesh-voxelization
	for (unsigned int i = 0; i < nV3; ++i) {
		for (unsigned int j = 0; j < nV2; ++j) {
			std::vector<glm::vec3> voxelStore;

			// find start/end feature voxels in this line...
			// voxels outside of these bounds have to remain outer voxels
			bool foundMin = false;
			unsigned int minK = 0;
			unsigned int maxK = 0;

			for (unsigned int k = 0; k < nV1; ++k) {
				if (FEATURE_CYAN == voxelClasses.at(i).at(j).at(k)) {
					maxK = k;
					if (!foundMin) {
						minK = k;
						foundMin = true;
					}
				}
			}

			for (unsigned int k = minK; k <= maxK; ++k) {
				// DIRECTION OF SCAN VECTOR IS eigenV1 basis vector

				if (FEATURE_CYAN == voxelClasses.at(i).at(j).at(k)) {
					if (glm::dot(voxelIntersectedFaceAvgNormals.at(i).at(j).at(k), eigenV1) > 0.0f) {

						// mark all stored voxels as inner...
						for (glm::vec3 const& voxel : voxelStore) {
							voxelClasses.at(voxel.x).at(voxel.y).at(voxel.z) = INNER_MAGENTA;
						}
					}
					voxelStore.clear();
				}
				else {
					voxelStore.push_back(glm::vec3(i, j, k));
				}
			}
		}
	}

	//NOTE: could test out what happens if we include feature voxels as well (i'm guessing it would make OBB slightly larger which could be beneficial
	// 10. GENERATE THE POINT SET P = {MODEL VERTS} + {INNER VOXEL BARYCENTRES}...


	std::vector<glm::vec3> pointSetP = pointSetM; // copy all unique model verts into our new set

	// add INNER VOXEL BARYCENTRES...
	// per voxel...
	for (unsigned int i = 0; i < nV3; ++i) {
		for (unsigned int j = 0; j < nV2; ++j) {
			for (unsigned int k = 0; k < nV1; ++k) {
				// if voxel was marked as INNER...
				if (INNER_MAGENTA == voxelClasses.at(i).at(j).at(k)) {
					float const v1Coord = (k + 0.5) * voxelSize + expandedMinScalarAlongV1;
					float const v2Coord = (j + 0.5) * voxelSize + expandedMinScalarAlongV2;
					float const v3Coord = (i + 0.5) * voxelSize + expandedMinScalarAlongV3;
					glm::vec3 const barycentre = v1Coord * eigenV1 + v2Coord * eigenV2 + v3Coord * eigenV3;
					pointSetP.push_back(barycentre);
				}
			}
		}
	}

/*
	//TEMP TESTING THIS WITH FEATURE VOXELS INCLUDED
	//NOTE: INCLUDING FEATURE VOXELS SCREWS UP OBBs
	//NOTE: this explodes RAM for armadillo
	std::vector<glm::vec3> pointSetP = pointSetM;

	for (unsigned int i = 0; i < nV3; ++i) {
		for (unsigned int j = 0; j < nV2; ++j) {
			for (unsigned int k = 0; k < nV1; ++k) {
				if (INNER_MAGENTA == voxelClasses.at(i).at(j).at(k) || FEATURE_CYAN == voxelClasses.at(i).at(j).at(k)) {
					float const v1Coord = (k + 0.5) * voxelSize + expandedMinScalarAlongV1;
					float const v2Coord = (j + 0.5) * voxelSize + expandedMinScalarAlongV2;
					float const v3Coord = (i + 0.5) * voxelSize + expandedMinScalarAlongV3;
					glm::vec3 const barycentre = v1Coord * eigenV1 + v2Coord * eigenV2 + v3Coord * eigenV3;
					pointSetP.push_back(barycentre);
				}
			}
		}
	}
*/
	return pointSetP;

/*
	// INITIAL OBB CAN BE DRAWN WITH THIS...
	// clear old cage if any...
	clearCage();

	//TESTING FOR NOW!!!!
	m_cage = std::make_shared<MeshObject>();
	m_cage->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	m_cage->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
	m_cage->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	m_cage->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
	m_cage->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	m_cage->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
	m_cage->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	m_cage->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);

	//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
	//THUS THE WINDING HERE MIGHT BE SCREWED UP!

	//TODO: simplify this triangulation into an algorithm (especially one that could allow easy reroval of indices making up the merged triangle faces of 2 side-by-side OBBs in tree)...

	// FROM 0 (000)

	m_cage->drawFaces.push_back(0);
	m_cage->drawFaces.push_back(2);
	m_cage->drawFaces.push_back(6);

	m_cage->drawFaces.push_back(0);
	m_cage->drawFaces.push_back(6);
	m_cage->drawFaces.push_back(4);


	m_cage->drawFaces.push_back(0);
	m_cage->drawFaces.push_back(1);
	m_cage->drawFaces.push_back(3);

	m_cage->drawFaces.push_back(0);
	m_cage->drawFaces.push_back(3);
	m_cage->drawFaces.push_back(2);


	m_cage->drawFaces.push_back(0);
	m_cage->drawFaces.push_back(4);
	m_cage->drawFaces.push_back(5);

	m_cage->drawFaces.push_back(0);
	m_cage->drawFaces.push_back(5);
	m_cage->drawFaces.push_back(1);

	// FROM 7 (111)

	m_cage->drawFaces.push_back(7);
	m_cage->drawFaces.push_back(3);
	m_cage->drawFaces.push_back(1);

	m_cage->drawFaces.push_back(7);
	m_cage->drawFaces.push_back(1);
	m_cage->drawFaces.push_back(5);


	m_cage->drawFaces.push_back(7);
	m_cage->drawFaces.push_back(5);
	m_cage->drawFaces.push_back(4);

	m_cage->drawFaces.push_back(7);
	m_cage->drawFaces.push_back(4);
	m_cage->drawFaces.push_back(6);


	m_cage->drawFaces.push_back(7);
	m_cage->drawFaces.push_back(6);
	m_cage->drawFaces.push_back(2);

	m_cage->drawFaces.push_back(7);
	m_cage->drawFaces.push_back(2);
	m_cage->drawFaces.push_back(3);


	// init vert colours (uniform light grey for now)
	for (unsigned int i = 0; i < m_cage->drawVerts.size(); ++i) {
		m_cage->colours.push_back(glm::vec3(0.8f, 0.8f, 0.8f));
	}

	// set cage black and also set picking colours...
	for (unsigned int i = 0; i < m_cage->colours.size(); ++i) {
		// render colour...
		m_cage->colours.at(i) = s_CAGE_UNSELECTED_COLOUR;

		// reference: http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
		// reference: https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_slow_easy.cpp
		// picking colour...
		//NOTE: this assumes that this cage will be the only object with picking colours (these colours must be unique)
		unsigned int const r = (i & 0x000000FF) >> 0;
		unsigned int const g = (i & 0x0000FF00) >> 8;
		unsigned int const b = (i & 0x00FF0000) >> 16;
		m_cage->pickingColours.push_back(glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f));
	}
	m_cage->m_polygonMode = PolygonMode::LINE; // set wireframe
	//m_cage->m_renderPoints = true; // hack to render the cage as points as well (2nd polygon mode)

	//m_cage->setScale(glm::vec3(0.02f, 0.02f, 0.02f));
	meshObjects.push_back(m_cage);
	renderEngine->assignBuffers(*m_cage);
*/

/*
	// USE THIS TO RENDER VOXELIZED MESH AS POINT SET P (MODEL VERTS + INNER VOXELS)
	// clear old cage if any...
	clearCage();
	m_cage = std::make_shared<MeshObject>();
	m_cage->drawVerts = pointSetP;

	// init faces as triangles that are actually points!!!
	for (unsigned int i = 0; i < m_cage->drawVerts.size(); ++i) {
		m_cage->drawFaces.push_back(i);
		m_cage->drawFaces.push_back(i);
		m_cage->drawFaces.push_back(i);
	}

	// init vert colours (uniform light grey for now)
	for (unsigned int i = 0; i < m_cage->drawVerts.size(); ++i) {
		m_cage->colours.push_back(glm::vec3(0.8f, 0.8f, 0.8f));
	}

	// set cage black and also set picking colours...
	for (unsigned int i = 0; i < m_cage->colours.size(); ++i) {
		// render colour...
		m_cage->colours.at(i) = s_CAGE_UNSELECTED_COLOUR;

		// reference: http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
		// reference: https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_slow_easy.cpp
		// picking colour...
		//NOTE: this assumes that this cage will be the only object with picking colours (these colours must be unique)
		unsigned int const r = (i & 0x000000FF) >> 0;
		unsigned int const g = (i & 0x0000FF00) >> 8;
		unsigned int const b = (i & 0x00FF0000) >> 16;
		m_cage->pickingColours.push_back(glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f));
	}
	m_cage->m_polygonMode = PolygonMode::POINT;
	//m_cage->m_renderPoints = true; // hack to render the cage as points as well (2nd polygon mode)

	//m_cage->setScale(glm::vec3(0.02f, 0.02f, 0.02f));
	meshObjects.push_back(m_cage);
	renderEngine->assignBuffers(*m_cage);
*/
}


std::shared_ptr<MeshObject> Program::generateOBBs(std::vector<glm::vec3> &points, unsigned int recursiveDepth) {
	if (points.empty()) return nullptr;

	// 1. generate OBB of points using PCA...
	// reference: https://stackoverflow.com/questions/6189229/creating-oobb-from-points
	
	// 2. compute centroid mu of points...
	glm::vec3 mu = glm::vec3(0.0f, 0.0f, 0.0f);
	for (glm::vec3 const& p : points) {
		mu += p;
	}
	mu /= points.size();

	// 3. compute covariance matrix...
	glm::mat3 covarianceMatrix = glm::mat3(0.0f);
	for (glm::vec3 const& p : points) {
		glm::vec3 const dev = p - mu;
		glm::mat3 const covMat_i = glm::outerProduct(dev, dev); // dev * (dev)^T
		covarianceMatrix += covMat_i;
	}
	//NOTE: this matrix should be REAL and SYMMETRIC

	// 4. get sorted eigenvalues (ascending) + corresponding eigenvectors...

	// convert matrix form (glm -> eigen) to work with eigensolver...
	Eigen::Matrix3f covMatEigen;
	for (unsigned int i = 0; i < covarianceMatrix.length(); ++i) { // loop over columns
		for (unsigned int j = 0; j < covarianceMatrix[i].length(); ++j) { // loop over rows
			covMatEigen(j, i) = covarianceMatrix[i][j]; //NOTE: eigen(row, col) vs glm(col, row)
		}
	}

	// reference: https://stackoverflow.com/questions/50458712/c-find-eigenvalues-and-eigenvectors-of-matrix
	Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigenSolver;
	eigenSolver.compute(covMatEigen);
	Eigen::Vector3f eigenValues = eigenSolver.eigenvalues(); //NOTE: eigenvalues come sorted in ascending order
	Eigen::Matrix3f eigenVectors = eigenSolver.eigenvectors(); //NOTE: eigenvectors come sorted corresponding to returned eigenvalues and are normalized. This returned matrix also corresponds to P in the diagonalization formula A = P*D*P^-1. Here A =:= covariance matrix

	// convert eigenvector forms (eigen -> glm)...
	glm::vec3 eigenV1 = glm::vec3(eigenVectors(0, 0), eigenVectors(1, 0), eigenVectors(2, 0));
	glm::vec3 eigenV2 = glm::vec3(eigenVectors(0, 1), eigenVectors(1, 1), eigenVectors(2, 1));
	glm::vec3 eigenV3 = glm::vec3(eigenVectors(0, 2), eigenVectors(1, 2), eigenVectors(2, 2));

	

	//TEMP TESTING THIS
	//eigenV1 = m_eigenV1;
	//eigenV2 = m_eigenV2;
	//eigenV3 = m_eigenV3;

	if (0 == recursiveDepth) {
		m_eigenV1 = eigenV1;
		m_eigenV2 = eigenV2;
		m_eigenV3 = eigenV3;
	}
	else {
		eigenV1 = m_eigenV1;
		eigenV2 = m_eigenV2;
		eigenV3 = m_eigenV3;
	}

	
	// find the 8 bounding vertices by finding the min/max coordinates of the projected points along each of the 3 mutually orthonormal eigenvectors that form our basis...
	float minScalarAlongV1 = std::numeric_limits<float>::max();
	float maxScalarAlongV1 = std::numeric_limits<float>::lowest();
	float minScalarAlongV2 = std::numeric_limits<float>::max();
	float maxScalarAlongV2 = std::numeric_limits<float>::lowest();
	float minScalarAlongV3 = std::numeric_limits<float>::max();
	float maxScalarAlongV3 = std::numeric_limits<float>::lowest();
	for (glm::vec3 const& p : points) {
		// project along eigenV1 axis...
		float const projScalarV1 = glm::dot(p, eigenV1) / glm::length2(eigenV1);
		if (projScalarV1 < minScalarAlongV1) minScalarAlongV1 = projScalarV1;
		if (projScalarV1 > maxScalarAlongV1) maxScalarAlongV1 = projScalarV1;

		// project along eigenV2 axis...
		float const projScalarV2 = glm::dot(p, eigenV2) / glm::length2(eigenV2);
		if (projScalarV2 < minScalarAlongV2) minScalarAlongV2 = projScalarV2;
		if (projScalarV2 > maxScalarAlongV2) maxScalarAlongV2 = projScalarV2;

		// project along eigenV3 axis...
		float const projScalarV3 = glm::dot(p, eigenV3) / glm::length2(eigenV3);
		if (projScalarV3 < minScalarAlongV3) minScalarAlongV3 = projScalarV3;
		if (projScalarV3 > maxScalarAlongV3) maxScalarAlongV3 = projScalarV3;
	}

	// 4.5. sort the basis eigenvectors... eigenV1 < eigenV2 < eigenV3

	float const tempExtentV1 = maxScalarAlongV1 - minScalarAlongV1;
	float const tempExtentV2 = maxScalarAlongV2 - minScalarAlongV2;
	float const tempExtentV3 = maxScalarAlongV3 - minScalarAlongV3;

	glm::vec3 const tempEigenV1 = eigenV1;
	glm::vec3 const tempEigenV2 = eigenV2;
	glm::vec3 const tempEigenV3 = eigenV3;

	float const tempMinV1 = minScalarAlongV1;
	float const tempMaxV1 = maxScalarAlongV1;
	float const tempMinV2 = minScalarAlongV2;
	float const tempMaxV2 = maxScalarAlongV2;
	float const tempMinV3 = minScalarAlongV3;
	float const tempMaxV3 = maxScalarAlongV3;

	std::vector<float> toSort = { tempExtentV1, tempExtentV2, tempExtentV3 };
	std::sort(toSort.begin(), toSort.end()); // sort in ascending order

	//NOTE: THIS IS A VERY BAD WAY OF DOING THIS, SINCE TIES WILL BREAK IT!!! ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (tempExtentV1 == toSort.at(0)) {
		
	} else if (tempExtentV2 == toSort.at(0)) {
		eigenV1 = tempEigenV2;
		minScalarAlongV1 = tempMinV2;
		maxScalarAlongV1 = tempMaxV2;
	} else {
		eigenV1 = tempEigenV3;
		minScalarAlongV1 = tempMinV3;
		maxScalarAlongV1 = tempMaxV3;
	}

	if (tempExtentV1 == toSort.at(1)) {
		eigenV2 = tempEigenV1;
		minScalarAlongV2 = tempMinV1;
		maxScalarAlongV2 = tempMaxV1;
	} else if (tempExtentV2 == toSort.at(1)) {

	} else {
		eigenV2 = tempEigenV3;
		minScalarAlongV2 = tempMinV3;
		maxScalarAlongV2 = tempMaxV3;
	}

	if (tempExtentV1 == toSort.at(2)) {
		eigenV3 = tempEigenV1;
		minScalarAlongV3 = tempMinV1;
		maxScalarAlongV3 = tempMaxV1;
	} else if (tempExtentV2 == toSort.at(2)) {
		eigenV3 = tempEigenV2;
		minScalarAlongV3 = tempMinV2;
		maxScalarAlongV3 = tempMaxV2;
	} else {

	}

	


	// 5. compute a cubic voxel size (side length)...
	//TODO: make sure this is never 0. could clamp lower bound as epsilon or use something like 0.001
	//TESTING THIS...
	//float const avgExtent = ((maxScalarAlongV1 - minScalarAlongV1) + (maxScalarAlongV2 - minScalarAlongV2) + (maxScalarAlongV3 - minScalarAlongV3)) / 3;
	//float const voxelSize = avgExtent / 100; // NOTE: increase denominator in order to increase voxel resolution (voxel count) - NOTE: that scaling the denom causes a cubic scale to the voxel count (so RAM explodes pretty quickly)
	//float const voxelSize = 0.0139374686f;
	float const voxelSize = m_voxelSize;

	//TODO: pass in the same voxel size!!!!!!!!!!!!!!!!!!!!

	// 6. voxelize our OBB into a slightly larger (or if lucky same size) 3D grid of cubes...
	// this expanded voxelized OBB will have new min/max scalars along each of the 3 basis axes denoting boundaries
	//NOTE: both the OBB and expanded voxelized-OBB will still have same centroid

	float const midScalarAlongV1 = (minScalarAlongV1 + maxScalarAlongV1) / 2;
	float const midScalarAlongV2 = (minScalarAlongV2 + maxScalarAlongV2) / 2;
	float const midScalarAlongV3 = (minScalarAlongV3 + maxScalarAlongV3) / 2;

	unsigned int const voxelCountAlongHalfV1 = glm::ceil((maxScalarAlongV1 - midScalarAlongV1) / voxelSize);
	unsigned int const voxelCountAlongHalfV2 = glm::ceil((maxScalarAlongV2 - midScalarAlongV2) / voxelSize);
	unsigned int const voxelCountAlongHalfV3 = glm::ceil((maxScalarAlongV3 - midScalarAlongV3) / voxelSize);

	float const expandedHalfExtentV1 = voxelCountAlongHalfV1 * voxelSize;
	float const expandedHalfExtentV2 = voxelCountAlongHalfV2 * voxelSize;
	float const expandedHalfExtentV3 = voxelCountAlongHalfV3 * voxelSize;

	float const expandedMinScalarAlongV1 = midScalarAlongV1 - expandedHalfExtentV1;
	float const expandedMaxScalarAlongV1 = midScalarAlongV1 + expandedHalfExtentV1;
	float const expandedMinScalarAlongV2 = midScalarAlongV2 - expandedHalfExtentV2;
	float const expandedMaxScalarAlongV2 = midScalarAlongV2 + expandedHalfExtentV2;
	float const expandedMinScalarAlongV3 = midScalarAlongV3 - expandedHalfExtentV3;
	float const expandedMaxScalarAlongV3 = midScalarAlongV3 + expandedHalfExtentV3;

	unsigned int const nV1 = 2 * voxelCountAlongHalfV1;
	unsigned int const nV2 = 2 * voxelCountAlongHalfV2;
	unsigned int const nV3 = 2 * voxelCountAlongHalfV3;

	// TERMINATE IF OUR SLICE IS TOO THIN OR HIT MAX RECURSIVE DEPTH...
	if (recursiveDepth >= s_MAX_RECURSIVE_DEPTH || nV1 < 10 || nV2 < 10 || nV3 < 10) {
		// return OBB data as simple MeshObject...

		std::shared_ptr<MeshObject> obb = std::make_shared<MeshObject>();

		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);

		//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
		//THUS THE WINDING HERE MIGHT BE SCREWED UP!

		//TODO: simplify this triangulation into an algorithm (especially one that could allow easy reroval of indices making up the merged triangle faces of 2 side-by-side OBBs in tree)...

		// FROM 0 (000)

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(2);
		obb->drawFaces.push_back(6);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(6);
		obb->drawFaces.push_back(4);


		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(1);
		obb->drawFaces.push_back(3);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(3);
		obb->drawFaces.push_back(2);


		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(4);
		obb->drawFaces.push_back(5);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(5);
		obb->drawFaces.push_back(1);

		// FROM 7 (111)

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(3);
		obb->drawFaces.push_back(1);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(1);
		obb->drawFaces.push_back(5);


		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(5);
		obb->drawFaces.push_back(4);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(4);
		obb->drawFaces.push_back(6);


		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(6);
		obb->drawFaces.push_back(2);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(2);
		obb->drawFaces.push_back(3);

		return obb;
	}


	// 7. now insert each point in our point set into a voxel. Voxels can contain many points. By storing the actual point coordinates, we can both get a count and easily split later on if needed...
	std::vector<std::vector<std::vector<std::vector<glm::vec3>>>> voxelsContainingPoints(nV3, std::vector<std::vector<std::vector<glm::vec3>>>(nV2, std::vector<std::vector<glm::vec3>>(nV1, std::vector<glm::vec3>())));

	for (glm::vec3 const& p : points) {
		// get voxel index i,j,k for this point coordinate
		// [V3][V2][V1]

		// project the point into OBB frame (measured by scalars along each of the 3 basis eigenvectors)...
		// .x will be eigenV1 scalar, .y is V2, .z is V3
		glm::vec3 const pLocal = glm::vec3(glm::dot(p, eigenV1) / glm::length2(eigenV1), glm::dot(p, eigenV2) / glm::length2(eigenV2), glm::dot(p, eigenV3) / glm::length2(eigenV3));

		// check voxel its in (by mapping formula)
		// inverse mapping formula (position to voxel index)
		// i on V3, j on V2, k on V1
		// inverse map back to indices i,j,k (single voxel)...

		//TODO: need to handle top most voxels to also include ceil
		unsigned int const indexI = glm::floor((pLocal.z - expandedMinScalarAlongV3) / voxelSize);
		unsigned int const indexJ = glm::floor((pLocal.y - expandedMinScalarAlongV2) / voxelSize);
		unsigned int const indexK = glm::floor((pLocal.x - expandedMinScalarAlongV1) / voxelSize);

		// store point p in this voxel...
		voxelsContainingPoints.at(indexI).at(indexJ).at(indexK).push_back(p);
	}
	// clear point set now that we've moved all the points out
	points.clear();


	float const eta = 0.75f;
	float const zeta = 0.5f;
	float const t2 = (maxScalarAlongV1 - minScalarAlongV1) / (maxScalarAlongV3 - minScalarAlongV3);
	//unsigned int spliceIndex = 0;

	// 8. a) RUN SLICING ALGO OVER EIGENV3...

	bool terminatedV3 = false;
	{
		std::vector<unsigned int> fx(nV3, 0);
		std::vector<int> classifyFx(nV3, 0);

		for (unsigned int i = 0; i < nV3; ++i) {
			unsigned int area = 0;
			for (unsigned int j = 0; j < nV2; ++j) {
				for (unsigned int k = 0; k < nV1; ++k) {
					area += voxelsContainingPoints.at(i).at(j).at(k).size();
				}
			}
			fx.at(i) = area;
		}

		// handle boundaries, if we have something like 1, 1, 1, 1, 5 - then the 4th 1 should be marked as a local minimum
		// if we have something like 0, 0, 0, 1, 5 - then we trim the 0's and the 1 is NOT marked as a local minimum
		// if we have something like 0, 0, 0, 1, 1, 1, 1, 5 - then we trim the 0's and get same situation as 1, 1, 1, 1, 5 and treat 4th 1 as a local minimum

		int startIndex = -1;
		int endIndex = -1;

		// trim any leading 0's...
		for (unsigned int i = 0; i < nV3; ++i) {
			if (fx.at(i) > 0) {
				startIndex = i;
				break;
			}
		}
		// trim any trailing 0's...
		for (unsigned int i = nV3 - 1; i >= 0; --i) {
			if (fx.at(i) > 0) {
				endIndex = i;
				break;
			}
		}

		// TERMINATE...
		if (-1 == startIndex || -1 == endIndex || startIndex == endIndex) terminatedV3 = true;

		/*
				{
					// return OBB data as simple MeshObject...

					std::shared_ptr<MeshObject> obb = std::make_shared<MeshObject>();

					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);

					//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
					//THUS THE WINDING HERE MIGHT BE SCREWED UP!

					//TODO: simplify this triangulation into an algorithm (especially one that could allow easy reroval of indices making up the merged triangle faces of 2 side-by-side OBBs in tree)...

					// FROM 0 (000)

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(2);
					obb->drawFaces.push_back(6);

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(6);
					obb->drawFaces.push_back(4);


					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(1);
					obb->drawFaces.push_back(3);

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(3);
					obb->drawFaces.push_back(2);


					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(4);
					obb->drawFaces.push_back(5);

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(5);
					obb->drawFaces.push_back(1);

					// FROM 7 (111)

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(3);
					obb->drawFaces.push_back(1);

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(1);
					obb->drawFaces.push_back(5);


					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(5);
					obb->drawFaces.push_back(4);

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(4);
					obb->drawFaces.push_back(6);


					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(6);
					obb->drawFaces.push_back(2);

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(2);
					obb->drawFaces.push_back(3);

					return obb;
				}
		*/

		//TODO: prevent very small slices by preventing marking of local minima that are too close to boundaries along axis
		//NOTE: maxima are unnaffected
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


		if (!terminatedV3) {
			unsigned int localMinCount = 0;
			unsigned int minArea = std::numeric_limits<unsigned int>::max();
			unsigned int maxArea = 0;

			if (fx.at(startIndex) > fx.at(startIndex + 1)) {
				classifyFx.at(startIndex) = 1;
				if (fx.at(startIndex) > maxArea) maxArea = fx.at(startIndex);
				if (fx.at(startIndex) < minArea) minArea = fx.at(startIndex);
			}
			if (fx.at(endIndex) > fx.at(endIndex - 1)) {
				classifyFx.at(endIndex) = 1;
				if (fx.at(endIndex) > maxArea) maxArea = fx.at(endIndex);
				if (fx.at(endIndex) < minArea) minArea = fx.at(endIndex);
			}
			for (unsigned int i = startIndex + 1; i < endIndex; ++i) {
				//MAXIMUM...
				if (fx.at(i - 1) < fx.at(i) && fx.at(i) >= fx.at(i + 1)) {
					classifyFx.at(i) = 1;
					if (fx.at(i) > maxArea) maxArea = fx.at(i);
					if (fx.at(i) < minArea) minArea = fx.at(i);
				}
				else if (fx.at(i - 1) <= fx.at(i) && fx.at(i) > fx.at(i + 1)) {
					classifyFx.at(i) = 1;
					if (fx.at(i) > maxArea) maxArea = fx.at(i);
					if (fx.at(i) < minArea) minArea = fx.at(i);
				}
				//MINIMUM...
				else if (i >= startIndex + 10 && i <= endIndex - 10) {
					if (fx.at(i - 1) > fx.at(i) && fx.at(i) <= fx.at(i + 1)) {
						classifyFx.at(i) = -1;
						if (fx.at(i) > maxArea) maxArea = fx.at(i);
						if (fx.at(i) < minArea) minArea = fx.at(i);
						++localMinCount;
					}
					else if (fx.at(i - 1) >= fx.at(i) && fx.at(i) < fx.at(i + 1)) {
						classifyFx.at(i) = -1;
						if (fx.at(i) > maxArea) maxArea = fx.at(i);
						if (fx.at(i) < minArea) minArea = fx.at(i);
						++localMinCount;
					}
				}
			}

			float const t1 = ((float)minArea) / maxArea;

			if (0 == localMinCount || (t1 > eta && t2 > zeta)) terminatedV3 = true;

			if (!terminatedV3) {
				// OTHERWISE, CONTINUE!

				// find the biggest JUMP...
				//TODO: make sure our f(x) is not constant and thus have no minima
				unsigned int spliceIndex = 0;
				float maxDYDX = std::numeric_limits<float>::lowest();

				//unsigned int minAreaTEMPTEST = std::numeric_limits<unsigned int>::max();
				for (unsigned int i = 0; i < nV3; ++i) {
					// if at a minimum...
					if (-1 == classifyFx.at(i)) {

						//TEMP TESTING SPLITTING AT LOWEST AREA
						/*
						if (fx.at(i) < minAreaTEMPTEST) {
							minAreaTEMPTEST = fx.at(i);
							spliceIndex = i;
						}
						*/

						// find slope between this minimum and every other maximum...
						for (unsigned int j = 0; j < nV3; ++j) {
							if (1 == classifyFx.at(j)) {
								//TODO: check this to make sure unsigned int difference doesnt screw up
								//NOTE: if the minimum is higher up than maximum, the slope will be negative/0 and thus be too small to even consider
								int const dx = glm::abs<int>((int)j - (int)i);
								int const dy = fx.at(j) - fx.at(i);
								float const dydx = ((float)dy) / dx;

								//TODO: could handle ties differently
								if (dydx > maxDYDX) {
									maxDYDX = dydx;
									spliceIndex = i;
								}
							}
						}
					}
				}


				// SPLICE!!!

				std::vector<glm::vec3> lowPointSet;
				for (unsigned int i = 0; i <= spliceIndex; ++i) {
					for (unsigned int j = 0; j < nV2; ++j) {
						for (unsigned int k = 0; k < nV1; ++k) {
							for (glm::vec3 const& p : voxelsContainingPoints.at(i).at(j).at(k)) lowPointSet.push_back(p);
						}
					}
				}

				std::vector<glm::vec3> highPointSet;
				for (unsigned int i = spliceIndex+1; i < nV3; ++i) {
					for (unsigned int j = 0; j < nV2; ++j) {
						for (unsigned int k = 0; k < nV1; ++k) {
							for (glm::vec3 const& p : voxelsContainingPoints.at(i).at(j).at(k)) highPointSet.push_back(p);
						}
					}
				}


				voxelsContainingPoints.clear(); // free

				std::shared_ptr<MeshObject> lowOBBTree = generateOBBs(lowPointSet, recursiveDepth+1);
				std::shared_ptr<MeshObject> highOBBTree = generateOBBs(highPointSet, recursiveDepth+1);

				//TODO: stitch together 2 OBBTrees properly
				//TEMP ~~~~~~ for now, just combine them into 1 mesh object preserving all data (only have drawVerts/drawFaces)

				std::shared_ptr<MeshObject> stitchedOBBTree = std::make_shared<MeshObject>();
				stitchedOBBTree->drawVerts = lowOBBTree->drawVerts;
				stitchedOBBTree->drawVerts.insert(stitchedOBBTree->drawVerts.end(), highOBBTree->drawVerts.begin(), highOBBTree->drawVerts.end());

				stitchedOBBTree->drawFaces = lowOBBTree->drawFaces;
				for (GLuint const& f : highOBBTree->drawFaces) {
					// must update these face indices since the high OBB verts were appended to end of low OBB verts and thus have a shifted index
					stitchedOBBTree->drawFaces.push_back(f + lowOBBTree->drawVerts.size());
				}

				return stitchedOBBTree;
			}
		}
	}

	// TERMINATED V3, try V2...
	// 8. b) RUN SLICING ALGO OVER EIGENV2...

	bool terminatedV2 = false;
	{
		std::vector<unsigned int> fx(nV2, 0);
		std::vector<int> classifyFx(nV2, 0);

		for (unsigned int i = 0; i < nV2; ++i) {
			unsigned int area = 0;
			for (unsigned int j = 0; j < nV3; ++j) {
				for (unsigned int k = 0; k < nV1; ++k) {
					area += voxelsContainingPoints.at(j).at(i).at(k).size();
				}
			}
			fx.at(i) = area;
		}

		// handle boundaries, if we have something like 1, 1, 1, 1, 5 - then the 4th 1 should be marked as a local minimum
		// if we have something like 0, 0, 0, 1, 5 - then we trim the 0's and the 1 is NOT marked as a local minimum
		// if we have something like 0, 0, 0, 1, 1, 1, 1, 5 - then we trim the 0's and get same situation as 1, 1, 1, 1, 5 and treat 4th 1 as a local minimum

		int startIndex = -1;
		int endIndex = -1;

		// trim any leading 0's...
		for (unsigned int i = 0; i < nV2; ++i) {
			if (fx.at(i) > 0) {
				startIndex = i;
				break;
			}
		}
		// trim any trailing 0's...
		for (unsigned int i = nV2 - 1; i >= 0; --i) {
			if (fx.at(i) > 0) {
				endIndex = i;
				break;
			}
		}

		// TERMINATE...
		if (-1 == startIndex || -1 == endIndex || startIndex == endIndex) terminatedV2 = true;

		/*
				{
					// return OBB data as simple MeshObject...

					std::shared_ptr<MeshObject> obb = std::make_shared<MeshObject>();

					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);

					//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
					//THUS THE WINDING HERE MIGHT BE SCREWED UP!

					//TODO: simplify this triangulation into an algorithm (especially one that could allow easy reroval of indices making up the merged triangle faces of 2 side-by-side OBBs in tree)...

					// FROM 0 (000)

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(2);
					obb->drawFaces.push_back(6);

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(6);
					obb->drawFaces.push_back(4);


					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(1);
					obb->drawFaces.push_back(3);

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(3);
					obb->drawFaces.push_back(2);


					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(4);
					obb->drawFaces.push_back(5);

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(5);
					obb->drawFaces.push_back(1);

					// FROM 7 (111)

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(3);
					obb->drawFaces.push_back(1);

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(1);
					obb->drawFaces.push_back(5);


					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(5);
					obb->drawFaces.push_back(4);

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(4);
					obb->drawFaces.push_back(6);


					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(6);
					obb->drawFaces.push_back(2);

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(2);
					obb->drawFaces.push_back(3);

					return obb;
				}
		*/

		if (!terminatedV2) {
			unsigned int localMinCount = 0;
			unsigned int minArea = std::numeric_limits<unsigned int>::max();
			unsigned int maxArea = 0;

			if (fx.at(startIndex) > fx.at(startIndex + 1)) {
				classifyFx.at(startIndex) = 1;
				if (fx.at(startIndex) > maxArea) maxArea = fx.at(startIndex);
				if (fx.at(startIndex) < minArea) minArea = fx.at(startIndex);
			}
			if (fx.at(endIndex) > fx.at(endIndex - 1)) {
				classifyFx.at(endIndex) = 1;
				if (fx.at(endIndex) > maxArea) maxArea = fx.at(endIndex);
				if (fx.at(endIndex) < minArea) minArea = fx.at(endIndex);
			}
			for (unsigned int i = startIndex + 1; i < endIndex; ++i) {
				//MAXIMUM...
				if (fx.at(i - 1) < fx.at(i) && fx.at(i) >= fx.at(i + 1)) {
					classifyFx.at(i) = 1;
					if (fx.at(i) > maxArea) maxArea = fx.at(i);
					if (fx.at(i) < minArea) minArea = fx.at(i);
				}
				else if (fx.at(i - 1) <= fx.at(i) && fx.at(i) > fx.at(i + 1)) {
					classifyFx.at(i) = 1;
					if (fx.at(i) > maxArea) maxArea = fx.at(i);
					if (fx.at(i) < minArea) minArea = fx.at(i);
				}
				//MINIMUM...
				else if (i >= startIndex + 10 && i <= endIndex - 10) {
					if (fx.at(i - 1) > fx.at(i) && fx.at(i) <= fx.at(i + 1)) {
						classifyFx.at(i) = -1;
						if (fx.at(i) > maxArea) maxArea = fx.at(i);
						if (fx.at(i) < minArea) minArea = fx.at(i);
						++localMinCount;
					}
					else if (fx.at(i - 1) >= fx.at(i) && fx.at(i) < fx.at(i + 1)) {
						classifyFx.at(i) = -1;
						if (fx.at(i) > maxArea) maxArea = fx.at(i);
						if (fx.at(i) < minArea) minArea = fx.at(i);
						++localMinCount;
					}
				}
/*
				//MINIMUM...
				else if (fx.at(i - 1) > fx.at(i) && fx.at(i) <= fx.at(i + 1)) {
					classifyFx.at(i) = -1;
					if (fx.at(i) > maxArea) maxArea = fx.at(i);
					if (fx.at(i) < minArea) minArea = fx.at(i);
					++localMinCount;
				}
				else if (fx.at(i - 1) >= fx.at(i) && fx.at(i) < fx.at(i + 1)) {
					classifyFx.at(i) = -1;
					if (fx.at(i) > maxArea) maxArea = fx.at(i);
					if (fx.at(i) < minArea) minArea = fx.at(i);
					++localMinCount;
				}
*/
			}

			float const t1 = ((float)minArea) / maxArea;

			if (0 == localMinCount || (t1 > eta && t2 > zeta)) terminatedV2 = true;

			if (!terminatedV2) {
				// OTHERWISE, CONTINUE!

				// find the biggest JUMP...
				//TODO: make sure our f(x) is not constant and thus have no minima
				unsigned int spliceIndex = 0;
				float maxDYDX = std::numeric_limits<float>::lowest();

				//unsigned int minAreaTEMPTEST = std::numeric_limits<unsigned int>::max();
				for (unsigned int i = 0; i < nV2; ++i) {
					// if at a minimum...
					if (-1 == classifyFx.at(i)) {

						//TEMP TESTING SPLITTING AT LOWEST AREA
						/*
						if (fx.at(i) < minAreaTEMPTEST) {
							minAreaTEMPTEST = fx.at(i);
							spliceIndex = i;
						}
						*/

						// find slope between this minimum and every other maximum...
						for (unsigned int j = 0; j < nV2; ++j) {
							if (1 == classifyFx.at(j)) {
								//TODO: check this to make sure unsigned int difference doesnt screw up
								//NOTE: if the minimum is higher up than maximum, the slope will be negative/0 and thus be too small to even consider
								int const dx = glm::abs<int>((int)j - (int)i);
								int const dy = fx.at(j) - fx.at(i);
								float const dydx = ((float)dy) / dx;

								//TODO: could handle ties differently
								if (dydx > maxDYDX) {
									maxDYDX = dydx;
									spliceIndex = i;
								}
							}
						}
					}
				}


				// SPLICE!!!

				std::vector<glm::vec3> lowPointSet;
				for (unsigned int i = 0; i <= spliceIndex; ++i) {
					for (unsigned int j = 0; j < nV3; ++j) {
						for (unsigned int k = 0; k < nV1; ++k) {
							for (glm::vec3 const& p : voxelsContainingPoints.at(j).at(i).at(k)) lowPointSet.push_back(p);
						}
					}
				}

				std::vector<glm::vec3> highPointSet;
				for (unsigned int i = spliceIndex+1; i < nV2; ++i) {
					for (unsigned int j = 0; j < nV3; ++j) {
						for (unsigned int k = 0; k < nV1; ++k) {
							for (glm::vec3 const& p : voxelsContainingPoints.at(j).at(i).at(k)) highPointSet.push_back(p);
						}
					}
				}


				voxelsContainingPoints.clear(); // free

				std::shared_ptr<MeshObject> lowOBBTree = generateOBBs(lowPointSet, recursiveDepth+1);
				std::shared_ptr<MeshObject> highOBBTree = generateOBBs(highPointSet, recursiveDepth+1);

				//TODO: stitch together 2 OBBTrees properly
				//TEMP ~~~~~~ for now, just combine them into 1 mesh object preserving all data (only have drawVerts/drawFaces)

				std::shared_ptr<MeshObject> stitchedOBBTree = std::make_shared<MeshObject>();
				stitchedOBBTree->drawVerts = lowOBBTree->drawVerts;
				stitchedOBBTree->drawVerts.insert(stitchedOBBTree->drawVerts.end(), highOBBTree->drawVerts.begin(), highOBBTree->drawVerts.end());

				stitchedOBBTree->drawFaces = lowOBBTree->drawFaces;
				for (GLuint const& f : highOBBTree->drawFaces) {
					// must update these face indices since the high OBB verts were appended to end of low OBB verts and thus have a shifted index
					stitchedOBBTree->drawFaces.push_back(f + lowOBBTree->drawVerts.size());
				}

				return stitchedOBBTree;
			}
		}
	}


	// TERMINATED V3 and V2, try V1...
	// 8. c) RUN SLICING ALGO OVER EIGENV1...

	bool terminatedV1 = false;
	{
		std::vector<unsigned int> fx(nV1, 0);
		std::vector<int> classifyFx(nV1, 0);

		for (unsigned int i = 0; i < nV1; ++i) {
			unsigned int area = 0;
			for (unsigned int j = 0; j < nV2; ++j) {
				for (unsigned int k = 0; k < nV3; ++k) {
					area += voxelsContainingPoints.at(k).at(j).at(i).size();
				}
			}
			fx.at(i) = area;
		}

		// handle boundaries, if we have something like 1, 1, 1, 1, 5 - then the 4th 1 should be marked as a local minimum
		// if we have something like 0, 0, 0, 1, 5 - then we trim the 0's and the 1 is NOT marked as a local minimum
		// if we have something like 0, 0, 0, 1, 1, 1, 1, 5 - then we trim the 0's and get same situation as 1, 1, 1, 1, 5 and treat 4th 1 as a local minimum

		int startIndex = -1;
		int endIndex = -1;

		// trim any leading 0's...
		for (unsigned int i = 0; i < nV1; ++i) {
			if (fx.at(i) > 0) {
				startIndex = i;
				break;
			}
		}
		// trim any trailing 0's...
		for (unsigned int i = nV1 - 1; i >= 0; --i) {
			if (fx.at(i) > 0) {
				endIndex = i;
				break;
			}
		}

		// TERMINATE...
		if (-1 == startIndex || -1 == endIndex || startIndex == endIndex) terminatedV1 = true;

		/*
				{
					// return OBB data as simple MeshObject...

					std::shared_ptr<MeshObject> obb = std::make_shared<MeshObject>();

					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
					obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);

					//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
					//THUS THE WINDING HERE MIGHT BE SCREWED UP!

					//TODO: simplify this triangulation into an algorithm (especially one that could allow easy reroval of indices making up the merged triangle faces of 2 side-by-side OBBs in tree)...

					// FROM 0 (000)

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(2);
					obb->drawFaces.push_back(6);

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(6);
					obb->drawFaces.push_back(4);


					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(1);
					obb->drawFaces.push_back(3);

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(3);
					obb->drawFaces.push_back(2);


					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(4);
					obb->drawFaces.push_back(5);

					obb->drawFaces.push_back(0);
					obb->drawFaces.push_back(5);
					obb->drawFaces.push_back(1);

					// FROM 7 (111)

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(3);
					obb->drawFaces.push_back(1);

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(1);
					obb->drawFaces.push_back(5);


					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(5);
					obb->drawFaces.push_back(4);

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(4);
					obb->drawFaces.push_back(6);


					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(6);
					obb->drawFaces.push_back(2);

					obb->drawFaces.push_back(7);
					obb->drawFaces.push_back(2);
					obb->drawFaces.push_back(3);

					return obb;
				}
		*/

		if (!terminatedV1) {
			unsigned int localMinCount = 0;
			unsigned int minArea = std::numeric_limits<unsigned int>::max();
			unsigned int maxArea = 0;

			if (fx.at(startIndex) > fx.at(startIndex + 1)) {
				classifyFx.at(startIndex) = 1;
				if (fx.at(startIndex) > maxArea) maxArea = fx.at(startIndex);
				if (fx.at(startIndex) < minArea) minArea = fx.at(startIndex);
			}
			if (fx.at(endIndex) > fx.at(endIndex - 1)) {
				classifyFx.at(endIndex) = 1;
				if (fx.at(endIndex) > maxArea) maxArea = fx.at(endIndex);
				if (fx.at(endIndex) < minArea) minArea = fx.at(endIndex);
			}
			for (unsigned int i = startIndex + 1; i < endIndex; ++i) {
				//MAXIMUM...
				if (fx.at(i - 1) < fx.at(i) && fx.at(i) >= fx.at(i + 1)) {
					classifyFx.at(i) = 1;
					if (fx.at(i) > maxArea) maxArea = fx.at(i);
					if (fx.at(i) < minArea) minArea = fx.at(i);
				}
				else if (fx.at(i - 1) <= fx.at(i) && fx.at(i) > fx.at(i + 1)) {
					classifyFx.at(i) = 1;
					if (fx.at(i) > maxArea) maxArea = fx.at(i);
					if (fx.at(i) < minArea) minArea = fx.at(i);
				}
				//MINIMUM...
				else if (i >= startIndex + 10 && i <= endIndex - 10) {
					if (fx.at(i - 1) > fx.at(i) && fx.at(i) <= fx.at(i + 1)) {
						classifyFx.at(i) = -1;
						if (fx.at(i) > maxArea) maxArea = fx.at(i);
						if (fx.at(i) < minArea) minArea = fx.at(i);
						++localMinCount;
					}
					else if (fx.at(i - 1) >= fx.at(i) && fx.at(i) < fx.at(i + 1)) {
						classifyFx.at(i) = -1;
						if (fx.at(i) > maxArea) maxArea = fx.at(i);
						if (fx.at(i) < minArea) minArea = fx.at(i);
						++localMinCount;
					}
				}
/*
				//MINIMUM...
				else if (fx.at(i - 1) > fx.at(i) && fx.at(i) <= fx.at(i + 1)) {
					classifyFx.at(i) = -1;
					if (fx.at(i) > maxArea) maxArea = fx.at(i);
					if (fx.at(i) < minArea) minArea = fx.at(i);
					++localMinCount;
				}
				else if (fx.at(i - 1) >= fx.at(i) && fx.at(i) < fx.at(i + 1)) {
					classifyFx.at(i) = -1;
					if (fx.at(i) > maxArea) maxArea = fx.at(i);
					if (fx.at(i) < minArea) minArea = fx.at(i);
					++localMinCount;
				}
*/
			}

			float const t1 = ((float)minArea) / maxArea;

			if (0 == localMinCount || (t1 > eta && t2 > zeta)) terminatedV1 = true;

			if (!terminatedV1) {
				// OTHERWISE, CONTINUE!

				// find the biggest JUMP...
				//TODO: make sure our f(x) is not constant and thus have no minima
				unsigned int spliceIndex = 0;
				float maxDYDX = std::numeric_limits<float>::lowest();

				//unsigned int minAreaTEMPTEST = std::numeric_limits<unsigned int>::max();
				for (unsigned int i = 0; i < nV1; ++i) {
					// if at a minimum...
					if (-1 == classifyFx.at(i)) {

						//TEMP TESTING SPLITTING AT LOWEST AREA
						/*
						if (fx.at(i) < minAreaTEMPTEST) {
							minAreaTEMPTEST = fx.at(i);
							spliceIndex = i;
						}
						*/

						// find slope between this minimum and every other maximum...
						for (unsigned int j = 0; j < nV1; ++j) {
							if (1 == classifyFx.at(j)) {
								//TODO: check this to make sure unsigned int difference doesnt screw up
								//NOTE: if the minimum is higher up than maximum, the slope will be negative/0 and thus be too small to even consider
								int const dx = glm::abs<int>((int)j - (int)i);
								int const dy = fx.at(j) - fx.at(i);
								float const dydx = ((float)dy) / dx;

								//TODO: could handle ties differently
								if (dydx > maxDYDX) {
									maxDYDX = dydx;
									spliceIndex = i;
								}
							}
						}
					}
				}


				// SPLICE!!!

				std::vector<glm::vec3> lowPointSet;
				for (unsigned int i = 0; i <= spliceIndex; ++i) {
					for (unsigned int j = 0; j < nV2; ++j) {
						for (unsigned int k = 0; k < nV3; ++k) {
							for (glm::vec3 const& p : voxelsContainingPoints.at(k).at(j).at(i)) lowPointSet.push_back(p);
						}
					}
				}

				std::vector<glm::vec3> highPointSet;
				for (unsigned int i = spliceIndex+1; i < nV1; ++i) {
					for (unsigned int j = 0; j < nV2; ++j) {
						for (unsigned int k = 0; k < nV3; ++k) {
							for (glm::vec3 const& p : voxelsContainingPoints.at(k).at(j).at(i)) highPointSet.push_back(p);
						}
					}
				}


				voxelsContainingPoints.clear(); // free

				std::shared_ptr<MeshObject> lowOBBTree = generateOBBs(lowPointSet, recursiveDepth+1);
				std::shared_ptr<MeshObject> highOBBTree = generateOBBs(highPointSet, recursiveDepth+1);

				//TODO: stitch together 2 OBBTrees properly
				//TEMP ~~~~~~ for now, just combine them into 1 mesh object preserving all data (only have drawVerts/drawFaces)

				std::shared_ptr<MeshObject> stitchedOBBTree = std::make_shared<MeshObject>();
				stitchedOBBTree->drawVerts = lowOBBTree->drawVerts;
				stitchedOBBTree->drawVerts.insert(stitchedOBBTree->drawVerts.end(), highOBBTree->drawVerts.begin(), highOBBTree->drawVerts.end());

				stitchedOBBTree->drawFaces = lowOBBTree->drawFaces;
				for (GLuint const& f : highOBBTree->drawFaces) {
					// must update these face indices since the high OBB verts were appended to end of low OBB verts and thus have a shifted index
					stitchedOBBTree->drawFaces.push_back(f + lowOBBTree->drawVerts.size());
				}

				return stitchedOBBTree;
			}
		}
	}


	// TERMINATED V3, TERMINATED V2, TERMINATED V1
	// if you get here, then everything has terminated and you must just return the OBB...

	// return OBB data as simple MeshObject...

	std::shared_ptr<MeshObject> obb = std::make_shared<MeshObject>();

	obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
	obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
	obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
	obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);

	//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
	//THUS THE WINDING HERE MIGHT BE SCREWED UP!

	//TODO: simplify this triangulation into an algorithm (especially one that could allow easy reroval of indices making up the merged triangle faces of 2 side-by-side OBBs in tree)...

	// FROM 0 (000)

	obb->drawFaces.push_back(0);
	obb->drawFaces.push_back(2);
	obb->drawFaces.push_back(6);

	obb->drawFaces.push_back(0);
	obb->drawFaces.push_back(6);
	obb->drawFaces.push_back(4);


	obb->drawFaces.push_back(0);
	obb->drawFaces.push_back(1);
	obb->drawFaces.push_back(3);

	obb->drawFaces.push_back(0);
	obb->drawFaces.push_back(3);
	obb->drawFaces.push_back(2);


	obb->drawFaces.push_back(0);
	obb->drawFaces.push_back(4);
	obb->drawFaces.push_back(5);

	obb->drawFaces.push_back(0);
	obb->drawFaces.push_back(5);
	obb->drawFaces.push_back(1);

	// FROM 7 (111)

	obb->drawFaces.push_back(7);
	obb->drawFaces.push_back(3);
	obb->drawFaces.push_back(1);

	obb->drawFaces.push_back(7);
	obb->drawFaces.push_back(1);
	obb->drawFaces.push_back(5);


	obb->drawFaces.push_back(7);
	obb->drawFaces.push_back(5);
	obb->drawFaces.push_back(4);

	obb->drawFaces.push_back(7);
	obb->drawFaces.push_back(4);
	obb->drawFaces.push_back(6);


	obb->drawFaces.push_back(7);
	obb->drawFaces.push_back(6);
	obb->drawFaces.push_back(2);

	obb->drawFaces.push_back(7);
	obb->drawFaces.push_back(2);
	obb->drawFaces.push_back(3);

	return obb;


/*
			// return OBB data as simple MeshObject...

			std::shared_ptr<MeshObject> obb = std::make_shared<MeshObject>();

			obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
			obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
			obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
			obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
			obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
			obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
			obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
			obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);

			//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
			//THUS THE WINDING HERE MIGHT BE SCREWED UP!

			//TODO: simplify this triangulation into an algorithm (especially one that could allow easy reroval of indices making up the merged triangle faces of 2 side-by-side OBBs in tree)...

			// FROM 0 (000)

			obb->drawFaces.push_back(0);
			obb->drawFaces.push_back(2);
			obb->drawFaces.push_back(6);

			obb->drawFaces.push_back(0);
			obb->drawFaces.push_back(6);
			obb->drawFaces.push_back(4);


			obb->drawFaces.push_back(0);
			obb->drawFaces.push_back(1);
			obb->drawFaces.push_back(3);

			obb->drawFaces.push_back(0);
			obb->drawFaces.push_back(3);
			obb->drawFaces.push_back(2);


			obb->drawFaces.push_back(0);
			obb->drawFaces.push_back(4);
			obb->drawFaces.push_back(5);

			obb->drawFaces.push_back(0);
			obb->drawFaces.push_back(5);
			obb->drawFaces.push_back(1);

			// FROM 7 (111)

			obb->drawFaces.push_back(7);
			obb->drawFaces.push_back(3);
			obb->drawFaces.push_back(1);

			obb->drawFaces.push_back(7);
			obb->drawFaces.push_back(1);
			obb->drawFaces.push_back(5);


			obb->drawFaces.push_back(7);
			obb->drawFaces.push_back(5);
			obb->drawFaces.push_back(4);

			obb->drawFaces.push_back(7);
			obb->drawFaces.push_back(4);
			obb->drawFaces.push_back(6);


			obb->drawFaces.push_back(7);
			obb->drawFaces.push_back(6);
			obb->drawFaces.push_back(2);

			obb->drawFaces.push_back(7);
			obb->drawFaces.push_back(2);
			obb->drawFaces.push_back(3);

			return obb;
		}
*/
		





	// 8. check TERMINATION CONDITION...

	// 8.1. compute T1...
	//TODO: not sure if this termination condition must check all 3 scan directions, but for the time being, we'll just use eigenV3 scan direction
	// find the global min/max cross-sectional areas scanning along eigenV3 direction


	// 8.2. compute T2...
	//NOTE: using bounds of non-expanded OBB
	//float const t2 = (maxScalarAlongV1 - minScalarAlongV1) / (maxScalarAlongV3 - minScalarAlongV3);

	//TODO: later on let the user pass in the 2 termination constants (could just set private vars in Program.h)
	//float const eta = 0.8f;
	//float const zeta = 0.3f;


	// RUN SPLICING ALGO OVER EIGENV3...

/*
	unsigned int minArea = std::numeric_limits<unsigned int>::max();
	unsigned int maxArea = std::numeric_limits<unsigned int>::min();

	for (unsigned int i = 0; i < nV3; ++i) {
		unsigned int area = 0;
		for (unsigned int j = 0; j < nV2; ++j) {
			for (unsigned int k = 0; k < nV1; ++k) {
				area += voxelsContainingPoints.at(i).at(j).at(k).size();
			}
		}
		if (area > 0) {
			if (area < minArea) minArea = area;
			if (area > maxArea) maxArea = area;
		}
	}

	float const t1 = ((float)minArea) / maxArea;
*/

	// (BASE CASE) termination condition...
/*
	if (t1 > eta && t2 > zeta) {
		// return OBB data as simple MeshObject...

		std::shared_ptr<MeshObject> obb = std::make_shared<MeshObject>();

		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);

		//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
		//THUS THE WINDING HERE MIGHT BE SCREWED UP!

		//TODO: simplify this triangulation into an algorithm (especially one that could allow easy reroval of indices making up the merged triangle faces of 2 side-by-side OBBs in tree)...

		// FROM 0 (000)

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(2);
		obb->drawFaces.push_back(6);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(6);
		obb->drawFaces.push_back(4);


		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(1);
		obb->drawFaces.push_back(3);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(3);
		obb->drawFaces.push_back(2);


		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(4);
		obb->drawFaces.push_back(5);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(5);
		obb->drawFaces.push_back(1);

		// FROM 7 (111)

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(3);
		obb->drawFaces.push_back(1);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(1);
		obb->drawFaces.push_back(5);


		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(5);
		obb->drawFaces.push_back(4);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(4);
		obb->drawFaces.push_back(6);


		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(6);
		obb->drawFaces.push_back(2);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(2);
		obb->drawFaces.push_back(3);

		return obb;
	}
*/

	// get here if we haven't terminated yet
	// 9. SLICE the point set contained within this OBB into 2 subsets (DIVIDE)

	// find the x value where to splice (splice at local minimum x that meets jump conditions)

	// first, locate the local minima (must be between 2 maxima - either local or boundaries)

	// 0 for NEITHER, -1 FOR MINIMUM, 1 FOR MAXIMUM
	//TODO: can move this up into the global min/max scan as well
	
/*
	std::vector<unsigned int> fx(nV3, 0);
	std::vector<int> classifyFx(nV3, 0);

	for (unsigned int i = 0; i < nV3; ++i) {
		unsigned int area = 0;
		for (unsigned int j = 0; j < nV2; ++j) {
			for (unsigned int k = 0; k < nV1; ++k) {
				area += voxelsContainingPoints.at(i).at(j).at(k).size();
			}
		}
		fx.at(i) = area;
	}
*/

	//TODO: must handle flat lines!!!! (mark endpoints of flat line at same state (minimum or maximum)
	//TODO: if entire f(x) is constant, then just terminate!!! - this is highly unlikely, but possible with flat geometry

	// handle boundaries...
	//NOTE: i'm assuming nV3 (and all axes for that matter are double-digit?)
	//NOTE: here i'm assuming boundaries aren't flat (same equal area)
	//NOTE: what if area is 0! - it shouldnt be since its a boundary, but might be possible
	//if (fx.at(0) > fx.at(1)) classifyFx.at(0) = 1;
	//if (fx.at(nV3-1) > fx.at(nV3-2)) classifyFx.at(nV3-1) = 1;

/*
	for (unsigned int i = 1; i < nV3-1; ++i) {
		//MAXIMUM...
		if (fx.at(i - 1) < fx.at(i) && fx.at(i) >= fx.at(i + 1)) classifyFx.at(i) = 1;
		else if (fx.at(i - 1) <= fx.at(i) && fx.at(i) > fx.at(i + 1)) classifyFx.at(i) = 1;
		//MINIMUM...
		else if (fx.at(i - 1) > fx.at(i) && fx.at(i) <= fx.at(i + 1)) classifyFx.at(i) = -1;
		else if (fx.at(i - 1) >= fx.at(i) && fx.at(i) < fx.at(i + 1)) classifyFx.at(i) = -1;
	}

	// find the biggest JUMP...
	//TODO: make sure our f(x) is not constant and thus have no minima
	unsigned int spliceIndex = 0;
	float maxDYDX = std::numeric_limits<float>::min();

	//unsigned int minAreaTEMPTEST = std::numeric_limits<unsigned int>::max();
	for (unsigned int i = 0; i < nV3; ++i) {
		// if at a minimum...
		if (-1 == classifyFx.at(i)) {
*/
			//TEMP TESTING SPLITTING AT LOWEST AREA
			/*
			if (fx.at(i) < minAreaTEMPTEST) {
				minAreaTEMPTEST = fx.at(i);
				spliceIndex = i;
			}
			*/
/*
			// find slope between this minimum and every other maximum...
			for (unsigned int j = 0; j < nV3; ++j) {
				if (1 == classifyFx.at(j)) {
					//TODO: check this to make sure unsigned int difference doesnt screw up
					//NOTE: if the minimum is higher up than maximum, the slope will be negative/0 and thus be too small to even consider
					int const dx = glm::abs(j - i);
					int const dy = fx.at(j) - fx.at(i);
					float const dydx = ((float)dy) / dx;

					//TODO: could handle ties differently
					if (dydx > maxDYDX) {
						maxDYDX = dydx;
						spliceIndex = i;
					}
				}
			}
		}
	}
*/

/*
	// TERMINATE!!!
	if (maxDYDX < 10.0f) {
		// return OBB data as simple MeshObject...

		std::shared_ptr<MeshObject> obb = std::make_shared<MeshObject>();

		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);

		//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
		//THUS THE WINDING HERE MIGHT BE SCREWED UP!

		//TODO: simplify this triangulation into an algorithm (especially one that could allow easy reroval of indices making up the merged triangle faces of 2 side-by-side OBBs in tree)...

		// FROM 0 (000)

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(2);
		obb->drawFaces.push_back(6);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(6);
		obb->drawFaces.push_back(4);


		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(1);
		obb->drawFaces.push_back(3);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(3);
		obb->drawFaces.push_back(2);


		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(4);
		obb->drawFaces.push_back(5);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(5);
		obb->drawFaces.push_back(1);

		// FROM 7 (111)

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(3);
		obb->drawFaces.push_back(1);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(1);
		obb->drawFaces.push_back(5);


		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(5);
		obb->drawFaces.push_back(4);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(4);
		obb->drawFaces.push_back(6);


		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(6);
		obb->drawFaces.push_back(2);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(2);
		obb->drawFaces.push_back(3);

		return obb;
	}
*/

/*
	// SPLICE!!!

	std::vector<glm::vec3> lowPointSet;
	for (unsigned int i = 0; i < spliceIndex; ++i) {
		for (unsigned int j = 0; j < nV2; ++j) {
			for (unsigned int k = 0; k < nV1; ++k) {
				for (glm::vec3 const& p : voxelsContainingPoints.at(i).at(j).at(k)) lowPointSet.push_back(p);
			}
		}
	}

	std::vector<glm::vec3> highPointSet;
	for (unsigned int i = spliceIndex; i < nV3; ++i) {
		for (unsigned int j = 0; j < nV2; ++j) {
			for (unsigned int k = 0; k < nV1; ++k) {
				for (glm::vec3 const& p : voxelsContainingPoints.at(i).at(j).at(k)) highPointSet.push_back(p);
			}
		}
	}
*/
	//TEMP ~~~~~~~~~~~~gonna just add in the trivial slicing rule (through barycenter - plane with normal parallel to eigenV3)

/*
	std::vector<glm::vec3> lowPointSet;
	for (unsigned int i = 0; i < voxelCountAlongHalfV3; ++i) {
		for (unsigned int j = 0; j < nV2; ++j) {
			for (unsigned int k = 0; k < nV1; ++k) {
				for (glm::vec3 const& p : voxelsContainingPoints.at(i).at(j).at(k)) lowPointSet.push_back(p);
			}
		}
	}

	std::vector<glm::vec3> highPointSet;
	for (unsigned int i = voxelCountAlongHalfV3; i < nV3; ++i) {
		for (unsigned int j = 0; j < nV2; ++j) {
			for (unsigned int k = 0; k < nV1; ++k) {
				for (glm::vec3 const& p : voxelsContainingPoints.at(i).at(j).at(k)) highPointSet.push_back(p);
			}
		}
	}
*/

	//TESTING THIS TEMP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
	if (lowPointSet.size() < 5000 || highPointSet.size() < 5000) { // terminate
		// return OBB data as simple MeshObject...

		std::shared_ptr<MeshObject> obb = std::make_shared<MeshObject>();

		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
		obb->drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);

		//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
		//THUS THE WINDING HERE MIGHT BE SCREWED UP!

		//TODO: simplify this triangulation into an algorithm (especially one that could allow easy reroval of indices making up the merged triangle faces of 2 side-by-side OBBs in tree)...

		// FROM 0 (000)

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(2);
		obb->drawFaces.push_back(6);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(6);
		obb->drawFaces.push_back(4);


		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(1);
		obb->drawFaces.push_back(3);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(3);
		obb->drawFaces.push_back(2);


		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(4);
		obb->drawFaces.push_back(5);

		obb->drawFaces.push_back(0);
		obb->drawFaces.push_back(5);
		obb->drawFaces.push_back(1);

		// FROM 7 (111)

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(3);
		obb->drawFaces.push_back(1);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(1);
		obb->drawFaces.push_back(5);


		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(5);
		obb->drawFaces.push_back(4);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(4);
		obb->drawFaces.push_back(6);


		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(6);
		obb->drawFaces.push_back(2);

		obb->drawFaces.push_back(7);
		obb->drawFaces.push_back(2);
		obb->drawFaces.push_back(3);

		return obb;
	}
*/

/*
	voxelsContainingPoints.clear(); // free

	std::shared_ptr<MeshObject> lowOBBTree = generateOBBs(lowPointSet);
	std::shared_ptr<MeshObject> highOBBTree = generateOBBs(highPointSet);

	//TODO: stitch together 2 OBBTrees properly
	//TEMP ~~~~~~ for now, just combine them into 1 mesh object preserving all data (only have drawVerts/drawFaces)

	std::shared_ptr<MeshObject> stitchedOBBTree = std::make_shared<MeshObject>();
	stitchedOBBTree->drawVerts = lowOBBTree->drawVerts;
	stitchedOBBTree->drawVerts.insert(stitchedOBBTree->drawVerts.end(), highOBBTree->drawVerts.begin(), highOBBTree->drawVerts.end());
	
	stitchedOBBTree->drawFaces = lowOBBTree->drawFaces;
	for (GLuint const& f : highOBBTree->drawFaces) {
		// must update these face indices since the high OBB verts were appended to end of low OBB verts and thus have a shifted index
		stitchedOBBTree->drawFaces.push_back(f + lowOBBTree->drawVerts.size());
	}

	return stitchedOBBTree;
*/
	//return nullptr;
}






// SIMILAR TO IMPROVED OBB METHOD (XIAN, LIN, GAO)...
// reference: http://www.cad.zju.edu.cn/home/hwlin/pdf_files/Automatic-cage-generation-by-improved-OBBs-for-mesh-deformation.pdf
// reference: http://www-home.htwg-konstanz.de/~umlauf/Papers/cagesurvSinCom.pdf
// reference: https://hewjunwei.wordpress.com/2013/01/26/obb-generation-via-principal-component-analysis/
void Program::generateCage2() {
	if (nullptr == m_model) return;

	std::shared_ptr<MeshObject> out_obb = std::make_shared<MeshObject>();
	std::shared_ptr<MeshObject> out_pointSetP = std::make_shared<MeshObject>();
	std::vector<glm::vec3> pointSetP = generatePointSetP2(*out_obb, *out_pointSetP);
	std::vector<std::vector<std::vector<unsigned int>>> obbSpace = generateOBBSpace(pointSetP);
	MeshTree const meshTree = generateMeshTree(obbSpace, 0, obbSpace.at(0).at(0).size() - 1, 0, obbSpace.at(0).size() - 1, 0, obbSpace.size() - 1, 0);
	
	//TODO: construct cage out of meshTree for rendering (e.g. add colours/picking colours, flags, etc.)

	// clear old cage if any...
	clearCage();

	///////////////////////////////////////////////////////////////////////////////////////
	// RENDER ACTUAL CAGE...

//
	m_cage = std::make_shared<MeshObject>();
	
	//RECALL: .x for V1, .y for V2, .z for V3
	for (glm::vec3 const& p : meshTree.m_vertexCoords) {
		glm::vec3 const p_cage = (p.x * m_voxelSize + m_expandedMinScalarAlongV1) * m_eigenV1 + (p.y * m_voxelSize + m_expandedMinScalarAlongV2) * m_eigenV2 + (p.z * m_voxelSize + m_expandedMinScalarAlongV3) * m_eigenV3;
		m_cage->drawVerts.push_back(p_cage);
	}
	m_cage->drawFaces = meshTree.m_faceIndices;

	// init vert colours (uniform light grey for now)
	for (unsigned int i = 0; i < m_cage->drawVerts.size(); ++i) {
		m_cage->colours.push_back(glm::vec3(0.8f, 0.8f, 0.8f));
	}

	// set cage black and also set picking colours...
	for (unsigned int i = 0; i < m_cage->colours.size(); ++i) {
		// render colour...
		m_cage->colours.at(i) = s_CAGE_UNSELECTED_COLOUR;

		// reference: http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
		// reference: https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_slow_easy.cpp
		// picking colour...
		//NOTE: this assumes that this cage will be the only object with picking colours (these colours must be unique)
		unsigned int const r = (i & 0x000000FF) >> 0;
		unsigned int const g = (i & 0x0000FF00) >> 8;
		unsigned int const b = (i & 0x00FF0000) >> 16;
		m_cage->pickingColours.push_back(glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f));
	}
	m_cage->m_polygonMode = PolygonMode::LINE; // set wireframe
	//m_cage->m_renderPoints = true; // hack to render the cage as points as well (2nd polygon mode)

	meshObjects.push_back(m_cage);
	renderEngine->assignBuffers(*m_cage);
//

	///////////////////////////////////////////////////////////////////////////////////////
	// RENDER INITIAL OBB...

/*

	m_cage = out_obb;

	// init vert colours (uniform light grey for now)
	for (unsigned int i = 0; i < m_cage->drawVerts.size(); ++i) {
		m_cage->colours.push_back(glm::vec3(0.8f, 0.8f, 0.8f));
	}

	// set cage black and also set picking colours...
	for (unsigned int i = 0; i < m_cage->colours.size(); ++i) {
		// render colour...
		m_cage->colours.at(i) = s_CAGE_UNSELECTED_COLOUR;

		// reference: http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
		// reference: https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_slow_easy.cpp
		// picking colour...
		//NOTE: this assumes that this cage will be the only object with picking colours (these colours must be unique)
		unsigned int const r = (i & 0x000000FF) >> 0;
		unsigned int const g = (i & 0x0000FF00) >> 8;
		unsigned int const b = (i & 0x00FF0000) >> 16;
		m_cage->pickingColours.push_back(glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f));
	}
	m_cage->m_polygonMode = PolygonMode::LINE; // set wireframe

	meshObjects.push_back(m_cage);
	renderEngine->assignBuffers(*m_cage);
*/

	///////////////////////////////////////////////////////////////////////////////////////
	// RENDER POINT SET P...

/*
	m_cage = out_pointSetP;

	// init vert colours (uniform light grey for now)
	for (unsigned int i = 0; i < m_cage->drawVerts.size(); ++i) {
		m_cage->colours.push_back(glm::vec3(0.8f, 0.8f, 0.8f));
	}

	// set cage black and also set picking colours...
	for (unsigned int i = 0; i < m_cage->colours.size(); ++i) {
		// render colour...
		m_cage->colours.at(i) = s_CAGE_UNSELECTED_COLOUR;

		// reference: http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
		// reference: https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_slow_easy.cpp
		// picking colour...
		//NOTE: this assumes that this cage will be the only object with picking colours (these colours must be unique)
		unsigned int const r = (i & 0x000000FF) >> 0;
		unsigned int const g = (i & 0x0000FF00) >> 8;
		unsigned int const b = (i & 0x00FF0000) >> 16;
		m_cage->pickingColours.push_back(glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f));
	}
	m_cage->m_polygonMode = PolygonMode::POINT;

	meshObjects.push_back(m_cage);
	renderEngine->assignBuffers(*m_cage);
*/

	///////////////////////////////////////////////////////////////////////////////////////
}



std::vector<glm::vec3> Program::generatePointSetP2(MeshObject &out_obb, MeshObject &out_pointSetP) {
	if (nullptr == m_model) return std::vector<glm::vec3>();

	// 1. extract set of model verts (eliminate duplicates)...
	std::vector<glm::vec3> pointSetM;
	for (glm::vec3 const& p : m_model->drawVerts) {
		auto it = std::find(pointSetM.begin(), pointSetM.end(), p);
		if (pointSetM.end() == it) { // new unique point
			pointSetM.push_back(p);
		}
	}

	// 2. generate initial OBB O of pointSetM using Principal Component Analysis...

	// low covariance of 2 values means more independent (less correlated)
	// covariance(x,x) = variance(x)
	// covariance matrix A generalizes the concept of variance in multiple dimensions
	// we want to diagonalize the covariance matrix to make signals strong (big diagonal entries) and distortions weak (low (in this case 0) off-diagonal entries)
	// A is REAL and SYMMETRIC ===> guaranteed to have 3 eigenvectors in the similarity transformation (change of basis matrix) when we diagonalize A.
	// THE 3 EIGENVECTORS OF COVARIANCE MATRIX WILL MAKE UP ORIENTATION OF OBB
	// large eigenvalues mean large variance, thus align OBB along eigenvector corresponding to largest eigenvalue

	//TODO: could improve this algo by working on the convex hull points only (but it might not be worth it)
	//TODO: could improve this program by using a more exact OBB method (slower) for low vert count models.

	// reference: https://stackoverflow.com/questions/6189229/creating-oobb-from-points
	// 3. compute centroid mu of points...
	glm::vec3 mu = glm::vec3(0.0f, 0.0f, 0.0f);
	for (glm::vec3 const& p : pointSetM) {
		mu += p;
	}
	mu /= pointSetM.size();

	// 4. compute covariance matrix...
	glm::mat3 covarianceMatrix = glm::mat3(0.0f);
	for (glm::vec3 const& p : pointSetM) {
		glm::vec3 const dev = p - mu;
		glm::mat3 const covMat_i = glm::outerProduct(dev, dev); // dev * (dev)^T
		covarianceMatrix += covMat_i;
	}
	//NOTE: this matrix should be REAL and SYMMETRIC

	// 5. get sorted eigenvalues (ascending) + corresponding eigenvectors...

	// convert matrix form (glm -> eigen) to work with eigensolver...
	Eigen::Matrix3f covMatEigen;
	for (unsigned int i = 0; i < covarianceMatrix.length(); ++i) { // loop over columns
		for (unsigned int j = 0; j < covarianceMatrix[i].length(); ++j) { // loop over rows
			covMatEigen(j, i) = covarianceMatrix[i][j]; //NOTE: eigen(row, col) vs glm(col, row)
		}
	}

	// reference: https://stackoverflow.com/questions/50458712/c-find-eigenvalues-and-eigenvectors-of-matrix
	Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigenSolver;
	eigenSolver.compute(covMatEigen);
	Eigen::Vector3f eigenValues = eigenSolver.eigenvalues(); //NOTE: eigenvalues come sorted in ascending order
	Eigen::Matrix3f eigenVectors = eigenSolver.eigenvectors(); //NOTE: eigenvectors come sorted corresponding to returned eigenvalues and are normalized. This returned matrix also corresponds to P in the diagonalization formula A = P*D*P^-1. Here A =:= covariance matrix

	// convert eigenvector forms (eigen -> glm)...
	glm::vec3 eigenV1 = glm::vec3(eigenVectors(0, 0), eigenVectors(1, 0), eigenVectors(2, 0));
	glm::vec3 eigenV2 = glm::vec3(eigenVectors(0, 1), eigenVectors(1, 1), eigenVectors(2, 1));
	glm::vec3 eigenV3 = glm::vec3(eigenVectors(0, 2), eigenVectors(1, 2), eigenVectors(2, 2));

	// find the 8 bounding vertices by finding the min/max coordinates of the projected points along each of the 3 mutually orthonormal eigenvectors that form our basis...
	float minScalarAlongV1 = std::numeric_limits<float>::max();
	float maxScalarAlongV1 = std::numeric_limits<float>::lowest();
	float minScalarAlongV2 = std::numeric_limits<float>::max();
	float maxScalarAlongV2 = std::numeric_limits<float>::lowest();
	float minScalarAlongV3 = std::numeric_limits<float>::max();
	float maxScalarAlongV3 = std::numeric_limits<float>::lowest();
	for (glm::vec3 const& p : pointSetM) {
		// project along eigenV1 axis...
		float const projScalarV1 = glm::dot(p, eigenV1) / glm::length2(eigenV1);
		if (projScalarV1 < minScalarAlongV1) minScalarAlongV1 = projScalarV1;
		if (projScalarV1 > maxScalarAlongV1) maxScalarAlongV1 = projScalarV1;

		// project along eigenV2 axis...
		float const projScalarV2 = glm::dot(p, eigenV2) / glm::length2(eigenV2);
		if (projScalarV2 < minScalarAlongV2) minScalarAlongV2 = projScalarV2;
		if (projScalarV2 > maxScalarAlongV2) maxScalarAlongV2 = projScalarV2;

		// project along eigenV3 axis...
		float const projScalarV3 = glm::dot(p, eigenV3) / glm::length2(eigenV3);
		if (projScalarV3 < minScalarAlongV3) minScalarAlongV3 = projScalarV3;
		if (projScalarV3 > maxScalarAlongV3) maxScalarAlongV3 = projScalarV3;
	}

	// 5.5 sort basis vectors (v3 >= v2 >= V1)...
	std::vector<SortableAxis> axes = { SortableAxis(eigenV1, minScalarAlongV1, maxScalarAlongV1), SortableAxis(eigenV2, minScalarAlongV2, maxScalarAlongV2), SortableAxis(eigenV3, minScalarAlongV3, maxScalarAlongV3) };
	std::sort(axes.begin(), axes.end());

	eigenV1 = axes.at(0).m_axis;
	eigenV2 = axes.at(1).m_axis;
	eigenV3 = axes.at(2).m_axis;

	minScalarAlongV1 = axes.at(0).m_min;
	minScalarAlongV2 = axes.at(1).m_min;
	minScalarAlongV3 = axes.at(2).m_min;

	maxScalarAlongV1 = axes.at(0).m_max;
	maxScalarAlongV2 = axes.at(1).m_max;
	maxScalarAlongV3 = axes.at(2).m_max;

	// 6. compute a cubic voxel size (side length)...
	float const avgExtent = ((maxScalarAlongV1 - minScalarAlongV1) + (maxScalarAlongV2 - minScalarAlongV2) + (maxScalarAlongV3 - minScalarAlongV3)) / 3;
	float const voxelSize = avgExtent / 100; // NOTE: increase denominator in order to increase voxel resolution (voxel count) - NOTE: that scaling the denom causes a cubic scale to the voxel count (so RAM explodes pretty quickly)
	m_voxelSize = voxelSize;

	// 7. voxelize our OBB into a slightly larger 3D grid of cubes...
	// this expanded voxelized OBB will have new min/max scalars along each of the 3 basis axes denoting boundaries
	//NOTE: both the OBB and expanded voxelized-OBB will still have same centroid

	float const midScalarAlongV1 = (minScalarAlongV1 + maxScalarAlongV1) / 2;
	float const midScalarAlongV2 = (minScalarAlongV2 + maxScalarAlongV2) / 2;
	float const midScalarAlongV3 = (minScalarAlongV3 + maxScalarAlongV3) / 2;

	//NOTE: adding 1 voxel to each side to handle edge cases
	unsigned int const voxelCountAlongHalfV1 = glm::ceil((maxScalarAlongV1 - midScalarAlongV1) / voxelSize) + 1;
	unsigned int const voxelCountAlongHalfV2 = glm::ceil((maxScalarAlongV2 - midScalarAlongV2) / voxelSize) + 1;
	unsigned int const voxelCountAlongHalfV3 = glm::ceil((maxScalarAlongV3 - midScalarAlongV3) / voxelSize) + 1;

	float const expandedHalfExtentV1 = voxelCountAlongHalfV1 * voxelSize;
	float const expandedHalfExtentV2 = voxelCountAlongHalfV2 * voxelSize;
	float const expandedHalfExtentV3 = voxelCountAlongHalfV3 * voxelSize;

	float const expandedMinScalarAlongV1 = midScalarAlongV1 - expandedHalfExtentV1;
	float const expandedMaxScalarAlongV1 = midScalarAlongV1 + expandedHalfExtentV1;
	float const expandedMinScalarAlongV2 = midScalarAlongV2 - expandedHalfExtentV2;
	float const expandedMaxScalarAlongV2 = midScalarAlongV2 + expandedHalfExtentV2;
	float const expandedMinScalarAlongV3 = midScalarAlongV3 - expandedHalfExtentV3;
	float const expandedMaxScalarAlongV3 = midScalarAlongV3 + expandedHalfExtentV3;

	// coloured voxel array (BLACK =:= OUTER (default), CYAN =:= FEATURE, MAGENTA =:= INNER)
	// those colours can be assigned later if we want to render the voxels, but for now treat BLACK == 0, CYAN == 1, MAGENTA = 2

	unsigned int const OUTER_BLACK = 0;
	unsigned int const FEATURE_CYAN = 1;
	unsigned int const INNER_MAGENTA = 2;

	unsigned int const nV1 = 2 * voxelCountAlongHalfV1;
	unsigned int const nV2 = 2 * voxelCountAlongHalfV2;
	unsigned int const nV3 = 2 * voxelCountAlongHalfV3;

	//TODO: fix the inner voxel issue
	// either 0, 1, or 2
	std::vector<std::vector<std::vector<unsigned int>>> voxelClasses(nV3, std::vector<std::vector<unsigned int>>(nV2, std::vector<unsigned int>(nV1, OUTER_BLACK)));
	//NOTE: below, this 3D array maps voxel [V3][V2][V1] with either glm::vec3(0.0f, 0.0f, 0.0f) or another non-zero vec3.
	//~~~~~ this vec3 will be non-zero if this voxel is found to be a FEATURE VOXEL (meaning it has intersected at least 1 triangle face of model).
	//~~~~~ if this feature voxel is found to intersect multiple triangle faces, then this vec3 is taken as the trivial average of the face normals (normalized sum of them)
	//TODO: there is a known issue with some voxels being wrongly marker INNER, this is likely due to the wrong normal being considered since we just take the average right now
	//~~~~~ this can be fixed/improved by instead using the face normal with intersection point with smallest projected scalar along eigenV1 (scan direction)
	std::vector<std::vector<std::vector<glm::vec3>>> voxelIntersectedFaceAvgNormals(nV3, std::vector<std::vector<glm::vec3>>(nV2, std::vector<glm::vec3>(nV1, glm::vec3(0.0f, 0.0f, 0.0f))));

	// 8. CLASSIFY FEATURE VOXELS...

	// per triangle...
	for (unsigned int f = 0; f < m_model->drawFaces.size(); f += 3) {
		// get 3 triangle verts in x,y,z coordinate frame...
		glm::vec3 const& tV1xyz = m_model->drawVerts.at(m_model->drawFaces.at(f));
		glm::vec3 const& tV2xyz = m_model->drawVerts.at(m_model->drawFaces.at(f + 1));
		glm::vec3 const& tV3xyz = m_model->drawVerts.at(m_model->drawFaces.at(f + 2));

		// project these 3 points into OBB frame (measured by scalars along each of the 3 basis eigenvectors)...
		// .x will be eigenV1 scalar, .y is V2, .z is V3
		glm::vec3 const tV1 = glm::vec3(glm::dot(tV1xyz, eigenV1) / glm::length2(eigenV1), glm::dot(tV1xyz, eigenV2) / glm::length2(eigenV2), glm::dot(tV1xyz, eigenV3) / glm::length2(eigenV3));
		glm::vec3 const tV2 = glm::vec3(glm::dot(tV2xyz, eigenV1) / glm::length2(eigenV1), glm::dot(tV2xyz, eigenV2) / glm::length2(eigenV2), glm::dot(tV2xyz, eigenV3) / glm::length2(eigenV3));
		glm::vec3 const tV3 = glm::vec3(glm::dot(tV3xyz, eigenV1) / glm::length2(eigenV1), glm::dot(tV3xyz, eigenV2) / glm::length2(eigenV2), glm::dot(tV3xyz, eigenV3) / glm::length2(eigenV3));

		// DISCRETIZE TRIANGLE FACE BY BARYCENTRIC COORDS...

		//NOTE: this will be susceptible to duplicates
		//TODO: make sure bounds actually are [0, 1] exactly (rather than stopping short before 1) - should be done now

		// NOTE: all these points will be in OBB space
		std::vector<glm::vec3> discreteTrianglePts;
		discreteTrianglePts.push_back(tV1);
		discreteTrianglePts.push_back(tV2);
		discreteTrianglePts.push_back(tV3);

		float const du = voxelSize / (glm::distance(tV1, glm::proj(tV1, tV3 - tV2)));
		float const dv = voxelSize / (glm::distance(tV2, glm::proj(tV2, tV3 - tV1)));

		for (float u = 0.0f; u <= 1.0f; u += glm::min<float>(du, 1.0f - u > 0.0f ? 1.0f - u : du)) {
			for (float v = 0.0f; v <= 1.0f - u; v += glm::min<float>(dv, 1.0f - u - v > 0.0f ? 1.0f - u - v : dv)) {
				float const w = 1.0f - u - v;
				discreteTrianglePts.push_back(u * tV1 + v * tV2 + w * tV3);
			}
		}

		// NOW FOR EVERY TRIANGLE POINT...
		// check voxel its in (by mapping formula) and mark FEATURE VOXEL, add to list of considered voxels for this tri-face and if not already considered add this face normal as contribution
		// inverse mapping formula (position to voxel index)
		// i on V3, j on V2, k on V1
		// storing already considered (by this face) feature voxels as vec3(i,j,k)

		std::vector<glm::vec3> consideredVoxels;
		for (glm::vec3 const& tPt : discreteTrianglePts) {
			// inverse map back to indices i,j,k (single voxel)...

			unsigned int const indexI = glm::floor((tPt.z - expandedMinScalarAlongV3) / voxelSize);
			unsigned int const indexJ = glm::floor((tPt.y - expandedMinScalarAlongV2) / voxelSize);
			unsigned int const indexK = glm::floor((tPt.x - expandedMinScalarAlongV1) / voxelSize);

			glm::vec3 const intersectedVoxel = glm::vec3(indexI, indexJ, indexK);
			auto it = std::find(consideredVoxels.begin(), consideredVoxels.end(), intersectedVoxel);
			if (consideredVoxels.end() == it) { // new considered voxel

				// mark voxel as FEATURE...
				voxelClasses.at(indexI).at(indexJ).at(indexK) = FEATURE_CYAN;

				// contribute this face's normal to voxel
				voxelIntersectedFaceAvgNormals.at(indexI).at(indexJ).at(indexK) += m_model->faceNormals.at(f / 3);

				// mark voxel as considered by this tri-face
				consideredVoxels.push_back(intersectedVoxel);
			}
		}
	}

	// 8.5. foreach voxel, normalize (avg) the contributed face normals

	// per voxel...
	for (unsigned int i = 0; i < nV3; ++i) {
		for (unsigned int j = 0; j < nV2; ++j) {
			for (unsigned int k = 0; k < nV1; ++k) {
				// if voxel was marked as FEATURE...
				if (FEATURE_CYAN == voxelClasses.at(i).at(j).at(k)) {
					//TODO: make sure the tri-face normals weren't symbolic 0 vector due to being really small???
					voxelIntersectedFaceAvgNormals.at(i).at(j).at(k) = glm::normalize(voxelIntersectedFaceAvgNormals.at(i).at(j).at(k));
				}
			}
		}
	}

	// 9. CLASSIFY INNER VOXELS (SCAN-ALGORITHM)...
	// reference: http://blog.wolfire.com/2009/11/Triangle-mesh-voxelization
	for (unsigned int i = 0; i < nV3; ++i) {
		for (unsigned int j = 0; j < nV2; ++j) {
			std::vector<glm::vec3> voxelStore;

			// find start/end feature voxels in this line...
			// voxels outside of these bounds have to remain outer voxels
			bool foundMin = false;
			unsigned int minK = 0;
			unsigned int maxK = 0;

			for (unsigned int k = 0; k < nV1; ++k) {
				if (FEATURE_CYAN == voxelClasses.at(i).at(j).at(k)) {
					maxK = k;
					if (!foundMin) {
						minK = k;
						foundMin = true;
					}
				}
			}

			for (unsigned int k = minK; k <= maxK; ++k) {
				// DIRECTION OF SCAN VECTOR IS eigenV1 basis vector

				if (FEATURE_CYAN == voxelClasses.at(i).at(j).at(k)) {
					if (glm::dot(voxelIntersectedFaceAvgNormals.at(i).at(j).at(k), eigenV1) > 0.0f) {

						// mark all stored voxels as inner...
						for (glm::vec3 const& voxel : voxelStore) {
							voxelClasses.at(voxel.x).at(voxel.y).at(voxel.z) = INNER_MAGENTA;
						}
					}
					voxelStore.clear();
				}
				else {
					voxelStore.push_back(glm::vec3(i, j, k));
				}
			}
		}
	}

	// 10. GENERATE THE POINT SET P = {MODEL VERTS} + {INNER VOXEL BARYCENTRES} + {FEATURE VOXEL BARYCENTRES}...
	//NOTE: i'm including feature voxels as well (differ from paper) since inner voxels may not exist for skinny parts of model (e.g. long skinny triangle faces)

	std::vector<glm::vec3> pointSetP = pointSetM; // copy all unique model verts into our new set

	// add INNER/FEATURE VOXEL BARYCENTRES...
	// per voxel...
	for (unsigned int i = 0; i < nV3; ++i) {
		for (unsigned int j = 0; j < nV2; ++j) {
			for (unsigned int k = 0; k < nV1; ++k) {
				// if voxel was marked as INNER/FEATURE...
				if (INNER_MAGENTA == voxelClasses.at(i).at(j).at(k) || FEATURE_CYAN == voxelClasses.at(i).at(j).at(k)) {
					float const v1Coord = (k + 0.5) * voxelSize + expandedMinScalarAlongV1;
					float const v2Coord = (j + 0.5) * voxelSize + expandedMinScalarAlongV2;
					float const v3Coord = (i + 0.5) * voxelSize + expandedMinScalarAlongV3;
					glm::vec3 const barycentre = v1Coord * eigenV1 + v2Coord * eigenV2 + v3Coord * eigenV3;
					pointSetP.push_back(barycentre);
				}
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////
	out_obb = MeshObject();

	out_obb.drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	out_obb.drawVerts.push_back(minScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
	out_obb.drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	out_obb.drawVerts.push_back(minScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
	out_obb.drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	out_obb.drawVerts.push_back(maxScalarAlongV1 * eigenV1 + minScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);
	out_obb.drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + minScalarAlongV3 * eigenV3);
	out_obb.drawVerts.push_back(maxScalarAlongV1 * eigenV1 + maxScalarAlongV2 * eigenV2 + maxScalarAlongV3 * eigenV3);

	//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
	//THUS THE WINDING HERE MIGHT BE SCREWED UP!

	// FROM 0 (000)

	out_obb.drawFaces.push_back(0);
	out_obb.drawFaces.push_back(2);
	out_obb.drawFaces.push_back(6);

	out_obb.drawFaces.push_back(0);
	out_obb.drawFaces.push_back(6);
	out_obb.drawFaces.push_back(4);


	out_obb.drawFaces.push_back(0);
	out_obb.drawFaces.push_back(1);
	out_obb.drawFaces.push_back(3);

	out_obb.drawFaces.push_back(0);
	out_obb.drawFaces.push_back(3);
	out_obb.drawFaces.push_back(2);


	out_obb.drawFaces.push_back(0);
	out_obb.drawFaces.push_back(4);
	out_obb.drawFaces.push_back(5);

	out_obb.drawFaces.push_back(0);
	out_obb.drawFaces.push_back(5);
	out_obb.drawFaces.push_back(1);

	// FROM 7 (111)

	out_obb.drawFaces.push_back(7);
	out_obb.drawFaces.push_back(3);
	out_obb.drawFaces.push_back(1);

	out_obb.drawFaces.push_back(7);
	out_obb.drawFaces.push_back(1);
	out_obb.drawFaces.push_back(5);


	out_obb.drawFaces.push_back(7);
	out_obb.drawFaces.push_back(5);
	out_obb.drawFaces.push_back(4);

	out_obb.drawFaces.push_back(7);
	out_obb.drawFaces.push_back(4);
	out_obb.drawFaces.push_back(6);


	out_obb.drawFaces.push_back(7);
	out_obb.drawFaces.push_back(6);
	out_obb.drawFaces.push_back(2);

	out_obb.drawFaces.push_back(7);
	out_obb.drawFaces.push_back(2);
	out_obb.drawFaces.push_back(3);
	///////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////
	out_pointSetP = MeshObject();

	out_pointSetP.drawVerts = pointSetP;

	// init faces as triangles that are actually points!!!
	for (unsigned int i = 0; i < out_pointSetP.drawVerts.size(); ++i) {
		out_pointSetP.drawFaces.push_back(i);
		out_pointSetP.drawFaces.push_back(i);
		out_pointSetP.drawFaces.push_back(i);
	}
	///////////////////////////////////////////////////////////////////////////////////////

	return pointSetP;
}





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::vector<std::vector<unsigned int>>> Program::generateOBBSpace(std::vector<glm::vec3> const& pointSetP) {
	if (pointSetP.empty()) return std::vector<std::vector<std::vector<unsigned int>>>();

	glm::vec3 eigenV1 = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 eigenV2 = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 eigenV3 = glm::vec3(0.0f, 0.0f, 0.0f);

	// recompute OBB, get 3 new axes that will be used for everything in the future...
	// 1. generate OBB of point set P using Principal Component Analysis...

	// low covariance of 2 values means more independent (less correlated)
	// covariance(x,x) = variance(x)
	// covariance matrix A generalizes the concept of variance in multiple dimensions
	// we want to diagonalize the covariance matrix to make signals strong (big diagonal entries) and distortions weak (low (in this case 0) off-diagonal entries)
	// A is REAL and SYMMETRIC ===> guaranteed to have 3 eigenvectors in the similarity transformation (change of basis matrix) when we diagonalize A.
	// THE 3 EIGENVECTORS OF COVARIANCE MATRIX WILL MAKE UP ORIENTATION OF OBB
	// large eigenvalues mean large variance, thus align OBB along eigenvector corresponding to largest eigenvalue

	//TODO: could improve this algo by working on the convex hull points only (but it might not be worth it)
	//TODO: could improve this program by using a more exact OBB method (slower) for low vert count models.

	// reference: https://stackoverflow.com/questions/6189229/creating-oobb-from-points
	// 2. compute centroid mu of points...
	glm::vec3 mu = glm::vec3(0.0f, 0.0f, 0.0f);
	for (glm::vec3 const& p : pointSetP) {
		mu += p;
	}
	mu /= pointSetP.size();

	// 3. compute covariance matrix...
	glm::mat3 covarianceMatrix = glm::mat3(0.0f);
	for (glm::vec3 const& p : pointSetP) {
		glm::vec3 const dev = p - mu;
		glm::mat3 const covMat_i = glm::outerProduct(dev, dev); // dev * (dev)^T
		covarianceMatrix += covMat_i;
	}
	//NOTE: this matrix should be REAL and SYMMETRIC

	// 4. get sorted eigenvalues (ascending) + corresponding eigenvectors...

	// convert matrix form (glm -> eigen) to work with eigensolver...
	Eigen::Matrix3f covMatEigen;
	for (unsigned int i = 0; i < covarianceMatrix.length(); ++i) { // loop over columns
		for (unsigned int j = 0; j < covarianceMatrix[i].length(); ++j) { // loop over rows
			covMatEigen(j, i) = covarianceMatrix[i][j]; //NOTE: eigen(row, col) vs glm(col, row)
		}
	}

	// reference: https://stackoverflow.com/questions/50458712/c-find-eigenvalues-and-eigenvectors-of-matrix
	Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigenSolver;
	eigenSolver.compute(covMatEigen);
	Eigen::Vector3f eigenValues = eigenSolver.eigenvalues(); //NOTE: eigenvalues come sorted in ascending order
	Eigen::Matrix3f eigenVectors = eigenSolver.eigenvectors(); //NOTE: eigenvectors come sorted corresponding to returned eigenvalues and are normalized. This returned matrix also corresponds to P in the diagonalization formula A = P*D*P^-1. Here A =:= covariance matrix

	// convert eigenvector forms (eigen -> glm)...
	eigenV1 = glm::vec3(eigenVectors(0, 0), eigenVectors(1, 0), eigenVectors(2, 0));
	eigenV2 = glm::vec3(eigenVectors(0, 1), eigenVectors(1, 1), eigenVectors(2, 1));
	eigenV3 = glm::vec3(eigenVectors(0, 2), eigenVectors(1, 2), eigenVectors(2, 2));

	// find the 8 bounding vertices by finding the min/max coordinates of the projected points along each of the 3 mutually orthonormal eigenvectors that form our basis...
	float minScalarAlongV1 = std::numeric_limits<float>::max();
	float maxScalarAlongV1 = std::numeric_limits<float>::lowest();
	float minScalarAlongV2 = std::numeric_limits<float>::max();
	float maxScalarAlongV2 = std::numeric_limits<float>::lowest();
	float minScalarAlongV3 = std::numeric_limits<float>::max();
	float maxScalarAlongV3 = std::numeric_limits<float>::lowest();
	for (glm::vec3 const& p : pointSetP) {
		// project along eigenV1 axis...
		float const projScalarV1 = glm::dot(p, eigenV1) / glm::length2(eigenV1);
		if (projScalarV1 < minScalarAlongV1) minScalarAlongV1 = projScalarV1;
		if (projScalarV1 > maxScalarAlongV1) maxScalarAlongV1 = projScalarV1;

		// project along eigenV2 axis...
		float const projScalarV2 = glm::dot(p, eigenV2) / glm::length2(eigenV2);
		if (projScalarV2 < minScalarAlongV2) minScalarAlongV2 = projScalarV2;
		if (projScalarV2 > maxScalarAlongV2) maxScalarAlongV2 = projScalarV2;

		// project along eigenV3 axis...
		float const projScalarV3 = glm::dot(p, eigenV3) / glm::length2(eigenV3);
		if (projScalarV3 < minScalarAlongV3) minScalarAlongV3 = projScalarV3;
		if (projScalarV3 > maxScalarAlongV3) maxScalarAlongV3 = projScalarV3;
	}

	// 4.5 sort basis vectors (v3 >= v2 >= V1)...
	std::vector<SortableAxis> axes = { SortableAxis(eigenV1, minScalarAlongV1, maxScalarAlongV1), SortableAxis(eigenV2, minScalarAlongV2, maxScalarAlongV2), SortableAxis(eigenV3, minScalarAlongV3, maxScalarAlongV3) };
	std::sort(axes.begin(), axes.end());

	eigenV1 = axes.at(0).m_axis;
	eigenV2 = axes.at(1).m_axis;
	eigenV3 = axes.at(2).m_axis;

	m_eigenV1 = eigenV1;
	m_eigenV2 = eigenV2;
	m_eigenV3 = eigenV3;

	minScalarAlongV1 = axes.at(0).m_min;
	minScalarAlongV2 = axes.at(1).m_min;
	minScalarAlongV3 = axes.at(2).m_min;

	maxScalarAlongV1 = axes.at(0).m_max;
	maxScalarAlongV2 = axes.at(1).m_max;
	maxScalarAlongV3 = axes.at(2).m_max;

	// 5. use voxel size of initial OBB and the parition length (side length)... 
	float const voxelSize = m_voxelSize;

	// 6. partition (pseudo-voxelize) our OBB into a slightly larger 3D grid of cubes...
	// this expanded voxelized OBB will have new min/max scalars along each of the 3 basis axes denoting boundaries
	//NOTE: both the OBB and expanded voxelized-OBB will still have same centroid

	float const midScalarAlongV1 = (minScalarAlongV1 + maxScalarAlongV1) / 2;
	float const midScalarAlongV2 = (minScalarAlongV2 + maxScalarAlongV2) / 2;
	float const midScalarAlongV3 = (minScalarAlongV3 + maxScalarAlongV3) / 2;

	//NOTE: adding 1 voxel to each side to handle edge cases
	unsigned int const voxelCountAlongHalfV1 = glm::ceil((maxScalarAlongV1 - midScalarAlongV1) / voxelSize) + 1;
	unsigned int const voxelCountAlongHalfV2 = glm::ceil((maxScalarAlongV2 - midScalarAlongV2) / voxelSize) + 1;
	unsigned int const voxelCountAlongHalfV3 = glm::ceil((maxScalarAlongV3 - midScalarAlongV3) / voxelSize) + 1;

	float const expandedHalfExtentV1 = voxelCountAlongHalfV1 * voxelSize;
	float const expandedHalfExtentV2 = voxelCountAlongHalfV2 * voxelSize;
	float const expandedHalfExtentV3 = voxelCountAlongHalfV3 * voxelSize;

	float const expandedMinScalarAlongV1 = midScalarAlongV1 - expandedHalfExtentV1;
	float const expandedMaxScalarAlongV1 = midScalarAlongV1 + expandedHalfExtentV1;
	float const expandedMinScalarAlongV2 = midScalarAlongV2 - expandedHalfExtentV2;
	float const expandedMaxScalarAlongV2 = midScalarAlongV2 + expandedHalfExtentV2;
	float const expandedMinScalarAlongV3 = midScalarAlongV3 - expandedHalfExtentV3;
	float const expandedMaxScalarAlongV3 = midScalarAlongV3 + expandedHalfExtentV3;

	m_expandedMinScalarAlongV1 = expandedMinScalarAlongV1;
	m_expandedMinScalarAlongV2 = expandedMinScalarAlongV2;
	m_expandedMinScalarAlongV3 = expandedMinScalarAlongV3;

	unsigned int const nV1 = 2 * voxelCountAlongHalfV1;
	unsigned int const nV2 = 2 * voxelCountAlongHalfV2;
	unsigned int const nV3 = 2 * voxelCountAlongHalfV3;

	// 7. compute the area of each voxel in our OBB space, (area = number of points in set P that fall in it)...
	// -  now insert each point in our point set into a voxel. Voxels can contain many points.
	std::vector<std::vector<std::vector<unsigned int>>> obbSpace(nV3, std::vector<std::vector<unsigned int>>(nV2, std::vector<unsigned int>(nV1, 0)));

	for (glm::vec3 const& p : pointSetP) {
		// get voxel index i,j,k for this point coordinate
		// [V3][V2][V1]

		// project the point into OBB frame (measured by scalars along each of the 3 basis eigenvectors)...
		// .x will be eigenV1 scalar, .y is V2, .z is V3
		glm::vec3 const pLocal = glm::vec3(glm::dot(p, eigenV1) / glm::length2(eigenV1), glm::dot(p, eigenV2) / glm::length2(eigenV2), glm::dot(p, eigenV3) / glm::length2(eigenV3));

		// check voxel its in (by mapping formula)
		// inverse mapping formula (position to voxel index)
		// i on V3, j on V2, k on V1
		// inverse map back to indices i,j,k (single voxel)...

		unsigned int const indexI = glm::floor((pLocal.z - expandedMinScalarAlongV3) / voxelSize);
		unsigned int const indexJ = glm::floor((pLocal.y - expandedMinScalarAlongV2) / voxelSize);
		unsigned int const indexK = glm::floor((pLocal.x - expandedMinScalarAlongV1) / voxelSize);

		// store point p in this voxel...
		++obbSpace.at(indexI).at(indexJ).at(indexK);
	}

	return obbSpace;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MeshTree Program::terminateMeshTree(unsigned int const minV1Index, unsigned int const maxV1Index, unsigned int const minV2Index, unsigned int const maxV2Index, unsigned int const minV3Index, unsigned int const maxV3Index) {
	MeshTree meshTree;

	//NOTE: must add 1 to max index to handle ceiling of bounding volume
	meshTree.m_vertexCoords.push_back(glm::vec3(minV1Index, minV2Index, minV3Index));
	meshTree.m_vertexCoords.push_back(glm::vec3(minV1Index, minV2Index, maxV3Index+1));
	meshTree.m_vertexCoords.push_back(glm::vec3(minV1Index, maxV2Index+1, minV3Index));
	meshTree.m_vertexCoords.push_back(glm::vec3(minV1Index, maxV2Index+1, maxV3Index+1));
	meshTree.m_vertexCoords.push_back(glm::vec3(maxV1Index+1, minV2Index, minV3Index));
	meshTree.m_vertexCoords.push_back(glm::vec3(maxV1Index+1, minV2Index, maxV3Index+1));
	meshTree.m_vertexCoords.push_back(glm::vec3(maxV1Index+1, maxV2Index+1, minV3Index));
	meshTree.m_vertexCoords.push_back(glm::vec3(maxV1Index+1, maxV2Index+1, maxV3Index+1));

	//TODO: idk the proper order of V1 x V2 x V3 for right-hand-rule, but thats to be figured out
	//THUS THE WINDING HERE MIGHT BE SCREWED UP!

	// FROM 0 (000)

	meshTree.m_faceIndices.push_back(0);
	meshTree.m_faceIndices.push_back(2);
	meshTree.m_faceIndices.push_back(6);

	meshTree.m_faceIndices.push_back(0);
	meshTree.m_faceIndices.push_back(6);
	meshTree.m_faceIndices.push_back(4);


	meshTree.m_faceIndices.push_back(0);
	meshTree.m_faceIndices.push_back(1);
	meshTree.m_faceIndices.push_back(3);

	meshTree.m_faceIndices.push_back(0);
	meshTree.m_faceIndices.push_back(3);
	meshTree.m_faceIndices.push_back(2);


	meshTree.m_faceIndices.push_back(0);
	meshTree.m_faceIndices.push_back(4);
	meshTree.m_faceIndices.push_back(5);

	meshTree.m_faceIndices.push_back(0);
	meshTree.m_faceIndices.push_back(5);
	meshTree.m_faceIndices.push_back(1);

	// FROM 7 (111)

	meshTree.m_faceIndices.push_back(7);
	meshTree.m_faceIndices.push_back(3);
	meshTree.m_faceIndices.push_back(1);

	meshTree.m_faceIndices.push_back(7);
	meshTree.m_faceIndices.push_back(1);
	meshTree.m_faceIndices.push_back(5);


	meshTree.m_faceIndices.push_back(7);
	meshTree.m_faceIndices.push_back(5);
	meshTree.m_faceIndices.push_back(4);

	meshTree.m_faceIndices.push_back(7);
	meshTree.m_faceIndices.push_back(4);
	meshTree.m_faceIndices.push_back(6);


	meshTree.m_faceIndices.push_back(7);
	meshTree.m_faceIndices.push_back(6);
	meshTree.m_faceIndices.push_back(2);

	meshTree.m_faceIndices.push_back(7);
	meshTree.m_faceIndices.push_back(2);
	meshTree.m_faceIndices.push_back(3);

	return meshTree;
}


MeshTree Program::generateMeshTree(std::vector<std::vector<std::vector<unsigned int>>> const& obbSpace, unsigned int minV1Index, unsigned int maxV1Index, unsigned int minV2Index, unsigned int maxV2Index, unsigned int minV3Index, unsigned int maxV3Index, unsigned int const recursiveDepth) {
	// TRIMMING...
	// we must resize the bounds to mimic an obb's tight bounds

	////////////////////////////////////////////////////////////////////////////////////////

	// check for trimming over V1...
	std::vector<unsigned int> fx1((maxV1Index - minV1Index) + 1, 0);
	for (unsigned int k = minV1Index; k <= maxV1Index; ++k) {
		unsigned int area = 0;
		for (unsigned int j = minV2Index; j <= maxV2Index; ++j) {
			for (unsigned int i = minV3Index; i <= maxV3Index; ++i) {
				area += obbSpace.at(i).at(j).at(k);
			}
		}
		fx1.at(k - minV1Index) = area;
	}

	// min bound...
	for (unsigned int i = 0; i < fx1.size(); ++i) {
		if (fx1.at(i) > 0) {
			minV1Index += i;
			break;
		}
	}
	
	// max bound...
	for (unsigned int i = fx1.size() - 1; i >= 0; --i) {
		if (fx1.at(i) > 0) {
			maxV1Index -= ((fx1.size() - 1) - i);
			break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////
	
	// check for trimming over V2...
	std::vector<unsigned int> fx2((maxV2Index - minV2Index) + 1, 0);
	for (unsigned int j = minV2Index; j <= maxV2Index; ++j) {
		unsigned int area = 0;
		for (unsigned int k = minV1Index; k <= maxV1Index; ++k) {
			for (unsigned int i = minV3Index; i <= maxV3Index; ++i) {
				area += obbSpace.at(i).at(j).at(k);
			}
		}
		fx2.at(j - minV2Index) = area;
	}

	// min bound...
	for (unsigned int i = 0; i < fx2.size(); ++i) {
		if (fx2.at(i) > 0) {
			minV2Index += i;
			break;
		}
	}

	// max bound...
	for (unsigned int i = fx2.size() - 1; i >= 0; --i) {
		if (fx2.at(i) > 0) {
			maxV2Index -= ((fx2.size() - 1) - i);
			break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////

	// check for trimming over V3...
	std::vector<unsigned int> fx3((maxV3Index - minV3Index) + 1, 0);
	for (unsigned int i = minV3Index; i <= maxV3Index; ++i) {
		unsigned int area = 0;
		for (unsigned int k = minV1Index; k <= maxV1Index; ++k) {
			for (unsigned int j = minV2Index; j <= maxV2Index; ++j) {
				area += obbSpace.at(i).at(j).at(k);
			}
		}
		fx3.at(i - minV3Index) = area;
	}

	// min bound...
	for (unsigned int i = 0; i < fx3.size(); ++i) {
		if (fx3.at(i) > 0) {
			minV3Index += i;
			break;
		}
	}

	// max bound...
	for (unsigned int i = fx3.size() - 1; i >= 0; --i) {
		if (fx3.at(i) > 0) {
			maxV3Index -= ((fx3.size() - 1) - i);
			break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////
	
	// TERMINATE...
	if (recursiveDepth >= m_maxRecursiveDepth) {
		return terminateMeshTree(minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
	}

	//NOTE: obbSpace is indexed [i][j][k] for i in V3, j in V2, k in V1

	// 0. preprocess order of axis-searching (search longest axis first)
	unsigned int const extentV1 = maxV1Index - minV1Index;
	unsigned int const extentV2 = maxV2Index - minV2Index;
	unsigned int const extentV3 = maxV3Index - minV3Index;

	int spliceIndex = -1;
	unsigned int const SPLIT_V1 = 1;
	unsigned int const SPLIT_V2 = 2;
	unsigned int const SPLIT_V3 = 3;

	unsigned int lastAxisSearched = 0;

	if (extentV1 >= extentV2 && extentV1 >= extentV3) {
		// search V1...
		spliceIndex = searchForSpliceIndexOverV1(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
		lastAxisSearched = SPLIT_V1;

		if (-1 == spliceIndex) {
			if (extentV2 >= extentV3) {
				// search V2...
				spliceIndex = searchForSpliceIndexOverV2(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
				lastAxisSearched = SPLIT_V2;

				if (-1 == spliceIndex) {
					// search V3...
					spliceIndex = searchForSpliceIndexOverV3(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
					lastAxisSearched = SPLIT_V3;
				}
			}
			else {
				// search V3...
				spliceIndex = searchForSpliceIndexOverV3(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
				lastAxisSearched = SPLIT_V3;

				if (-1 == spliceIndex) {
					// search V2...
					spliceIndex = searchForSpliceIndexOverV2(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
					lastAxisSearched = SPLIT_V2;
				}
			}
		}
	}
	else if (extentV2 >= extentV1 && extentV2 >= extentV3) {
		// search V2...
		spliceIndex = searchForSpliceIndexOverV2(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
		lastAxisSearched = SPLIT_V2;

		if (-1 == spliceIndex) {
			if (extentV1 >= extentV3) {
				// search V1...
				spliceIndex = searchForSpliceIndexOverV1(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
				lastAxisSearched = SPLIT_V1;

				if (-1 == spliceIndex) {
					// search V3...
					spliceIndex = searchForSpliceIndexOverV3(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
					lastAxisSearched = SPLIT_V3;
				}
			}
			else {
				// search V3...
				spliceIndex = searchForSpliceIndexOverV3(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
				lastAxisSearched = SPLIT_V3;

				if (-1 == spliceIndex) {
					// search V1...
					spliceIndex = searchForSpliceIndexOverV1(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
					lastAxisSearched = SPLIT_V1;
				}
			}
		}
	}
	else {
		// search V3...
		spliceIndex = searchForSpliceIndexOverV3(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
		lastAxisSearched = SPLIT_V3;

		if (-1 == spliceIndex) {
			if (extentV1 >= extentV2) {
				// search V1...
				spliceIndex = searchForSpliceIndexOverV1(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
				lastAxisSearched = SPLIT_V1;

				if (-1 == spliceIndex) {
					// search V2...
					spliceIndex = searchForSpliceIndexOverV2(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
					lastAxisSearched = SPLIT_V2;
				}
			}
			else {
				// search V2...
				spliceIndex = searchForSpliceIndexOverV2(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
				lastAxisSearched = SPLIT_V2;

				if (-1 == spliceIndex) {
					// search V1...
					spliceIndex = searchForSpliceIndexOverV1(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
					lastAxisSearched = SPLIT_V1;
				}
			}
		}
	}

	// TERMINATE...
	if (-1 == spliceIndex) {
		return terminateMeshTree(minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index);
	}


	// SPLICE...
	if (SPLIT_V1 == lastAxisSearched) {
		MeshTree lowMeshTree = generateMeshTree(obbSpace, minV1Index, spliceIndex, minV2Index, maxV2Index, minV3Index, maxV3Index, recursiveDepth + 1);
		MeshTree highMeshTree = generateMeshTree(obbSpace, spliceIndex + 1, maxV1Index, minV2Index, maxV2Index, minV3Index, maxV3Index, recursiveDepth + 1);

		// REMOVE FACES ALONG SPLIT PLANE...
/*
		std::vector<unsigned int> tempFacesLow;
		for (unsigned int f = 0; f < lowMeshTree.m_faceIndices.size(); f += 3) {
			if ((spliceIndex+1) == lowMeshTree.m_vertexCoords.at(lowMeshTree.m_faceIndices.at(f)).x && (spliceIndex + 1) == lowMeshTree.m_vertexCoords.at(lowMeshTree.m_faceIndices.at(f + 1)).x && (spliceIndex + 1) == lowMeshTree.m_vertexCoords.at(lowMeshTree.m_faceIndices.at(f + 2)).x) {
				// REMOVE FACE (3 indices)...
				// by not adding them
			}
			else {
				tempFacesLow.push_back(lowMeshTree.m_faceIndices.at(f));
				tempFacesLow.push_back(lowMeshTree.m_faceIndices.at(f+1));
				tempFacesLow.push_back(lowMeshTree.m_faceIndices.at(f+2));
			}
		}
		lowMeshTree.m_faceIndices = tempFacesLow;

		std::vector<unsigned int> tempFacesHigh;
		for (unsigned int f = 0; f < highMeshTree.m_faceIndices.size(); f += 3) {
			if ((spliceIndex + 1) == highMeshTree.m_vertexCoords.at(highMeshTree.m_faceIndices.at(f)).x && (spliceIndex + 1) == highMeshTree.m_vertexCoords.at(highMeshTree.m_faceIndices.at(f + 1)).x && (spliceIndex + 1) == highMeshTree.m_vertexCoords.at(highMeshTree.m_faceIndices.at(f + 2)).x) {
				// REMOVE FACE (3 indices)...
				// by not adding them
			}
			else {
				tempFacesHigh.push_back(highMeshTree.m_faceIndices.at(f));
				tempFacesHigh.push_back(highMeshTree.m_faceIndices.at(f + 1));
				tempFacesHigh.push_back(highMeshTree.m_faceIndices.at(f + 2));
			}
		}
		highMeshTree.m_faceIndices = tempFacesHigh;
*/

		// stitching...
		MeshTree stitchedTree;
		stitchedTree.m_vertexCoords = lowMeshTree.m_vertexCoords;
		stitchedTree.m_vertexCoords.insert(stitchedTree.m_vertexCoords.end(), highMeshTree.m_vertexCoords.begin(), highMeshTree.m_vertexCoords.end());

		stitchedTree.m_faceIndices = lowMeshTree.m_faceIndices;
		for (unsigned int const& f : highMeshTree.m_faceIndices) {
			// must update these face indices since the high OBB verts were appended to end of low OBB verts and thus have a shifted index
			stitchedTree.m_faceIndices.push_back(f + lowMeshTree.m_vertexCoords.size());
		}

		return stitchedTree;
	}
	else if (SPLIT_V2 == lastAxisSearched) {
		MeshTree lowMeshTree = generateMeshTree(obbSpace, minV1Index, maxV1Index, minV2Index, spliceIndex, minV3Index, maxV3Index, recursiveDepth + 1);
		MeshTree highMeshTree = generateMeshTree(obbSpace, minV1Index, maxV1Index, spliceIndex + 1, maxV2Index, minV3Index, maxV3Index, recursiveDepth + 1);

		// REMOVE FACES ALONG SPLIT PLANE...
/*
		std::vector<unsigned int> tempFacesLow;
		for (unsigned int f = 0; f < lowMeshTree.m_faceIndices.size(); f += 3) {
			if ((spliceIndex + 1) == lowMeshTree.m_vertexCoords.at(lowMeshTree.m_faceIndices.at(f)).y && (spliceIndex + 1) == lowMeshTree.m_vertexCoords.at(lowMeshTree.m_faceIndices.at(f + 1)).y && (spliceIndex + 1) == lowMeshTree.m_vertexCoords.at(lowMeshTree.m_faceIndices.at(f + 2)).y) {
				// REMOVE FACE (3 indices)...
				// by not adding them
			}
			else {
				tempFacesLow.push_back(lowMeshTree.m_faceIndices.at(f));
				tempFacesLow.push_back(lowMeshTree.m_faceIndices.at(f + 1));
				tempFacesLow.push_back(lowMeshTree.m_faceIndices.at(f + 2));
			}
		}
		lowMeshTree.m_faceIndices = tempFacesLow;

		std::vector<unsigned int> tempFacesHigh;
		for (unsigned int f = 0; f < highMeshTree.m_faceIndices.size(); f += 3) {
			if ((spliceIndex + 1) == highMeshTree.m_vertexCoords.at(highMeshTree.m_faceIndices.at(f)).y && (spliceIndex + 1) == highMeshTree.m_vertexCoords.at(highMeshTree.m_faceIndices.at(f + 1)).y && (spliceIndex + 1) == highMeshTree.m_vertexCoords.at(highMeshTree.m_faceIndices.at(f + 2)).y) {
				// REMOVE FACE (3 indices)...
				// by not adding them
			}
			else {
				tempFacesHigh.push_back(highMeshTree.m_faceIndices.at(f));
				tempFacesHigh.push_back(highMeshTree.m_faceIndices.at(f + 1));
				tempFacesHigh.push_back(highMeshTree.m_faceIndices.at(f + 2));
			}
		}
		highMeshTree.m_faceIndices = tempFacesHigh;
*/

		// stitching...
		MeshTree stitchedTree;
		stitchedTree.m_vertexCoords = lowMeshTree.m_vertexCoords;
		stitchedTree.m_vertexCoords.insert(stitchedTree.m_vertexCoords.end(), highMeshTree.m_vertexCoords.begin(), highMeshTree.m_vertexCoords.end());

		stitchedTree.m_faceIndices = lowMeshTree.m_faceIndices;
		for (unsigned int const& f : highMeshTree.m_faceIndices) {
			// must update these face indices since the high OBB verts were appended to end of low OBB verts and thus have a shifted index
			stitchedTree.m_faceIndices.push_back(f + lowMeshTree.m_vertexCoords.size());
		}

		return stitchedTree;
	}
	else if (SPLIT_V3 == lastAxisSearched) {
		MeshTree lowMeshTree = generateMeshTree(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, minV3Index, spliceIndex, recursiveDepth + 1);
		MeshTree highMeshTree = generateMeshTree(obbSpace, minV1Index, maxV1Index, minV2Index, maxV2Index, spliceIndex + 1, maxV3Index, recursiveDepth + 1);

		// REMOVE FACES ALONG SPLIT PLANE...
/*
		std::vector<unsigned int> tempFacesLow;
		for (unsigned int f = 0; f < lowMeshTree.m_faceIndices.size(); f += 3) {
			if ((spliceIndex + 1) == lowMeshTree.m_vertexCoords.at(lowMeshTree.m_faceIndices.at(f)).z && (spliceIndex + 1) == lowMeshTree.m_vertexCoords.at(lowMeshTree.m_faceIndices.at(f + 1)).z && (spliceIndex + 1) == lowMeshTree.m_vertexCoords.at(lowMeshTree.m_faceIndices.at(f + 2)).z) {
				// REMOVE FACE (3 indices)...
				// by not adding them
			}
			else {
				tempFacesLow.push_back(lowMeshTree.m_faceIndices.at(f));
				tempFacesLow.push_back(lowMeshTree.m_faceIndices.at(f + 1));
				tempFacesLow.push_back(lowMeshTree.m_faceIndices.at(f + 2));
			}
		}
		lowMeshTree.m_faceIndices = tempFacesLow;

		std::vector<unsigned int> tempFacesHigh;
		for (unsigned int f = 0; f < highMeshTree.m_faceIndices.size(); f += 3) {
			if ((spliceIndex + 1) == highMeshTree.m_vertexCoords.at(highMeshTree.m_faceIndices.at(f)).z && (spliceIndex + 1) == highMeshTree.m_vertexCoords.at(highMeshTree.m_faceIndices.at(f + 1)).z && (spliceIndex + 1) == highMeshTree.m_vertexCoords.at(highMeshTree.m_faceIndices.at(f + 2)).z) {
				// REMOVE FACE (3 indices)...
				// by not adding them
			}
			else {
				tempFacesHigh.push_back(highMeshTree.m_faceIndices.at(f));
				tempFacesHigh.push_back(highMeshTree.m_faceIndices.at(f + 1));
				tempFacesHigh.push_back(highMeshTree.m_faceIndices.at(f + 2));
			}
		}
		highMeshTree.m_faceIndices = tempFacesHigh;
*/

		// stitching...
		MeshTree stitchedTree;
		stitchedTree.m_vertexCoords = lowMeshTree.m_vertexCoords;
		stitchedTree.m_vertexCoords.insert(stitchedTree.m_vertexCoords.end(), highMeshTree.m_vertexCoords.begin(), highMeshTree.m_vertexCoords.end());

		stitchedTree.m_faceIndices = lowMeshTree.m_faceIndices;
		for (unsigned int const& f : highMeshTree.m_faceIndices) {
			// must update these face indices since the high OBB verts were appended to end of low OBB verts and thus have a shifted index
			stitchedTree.m_faceIndices.push_back(f + lowMeshTree.m_vertexCoords.size());
		}

		return stitchedTree;
	}
	else { //NOTE: this case should never happen
		return MeshTree();
	}
}


//NOTE: RETURN -1 on no index found
int Program::searchForSpliceIndexOverV1(std::vector<std::vector<std::vector<unsigned int>>> const& obbSpace, unsigned int const minV1Index, unsigned int const maxV1Index, unsigned int const minV2Index, unsigned int const maxV2Index, unsigned int const minV3Index, unsigned int const maxV3Index) {
	
	unsigned int const nV1 = (maxV1Index - minV1Index) + 1;
	unsigned int const nV2 = (maxV2Index - minV2Index) + 1;
	unsigned int const nV3 = (maxV3Index - minV3Index) + 1;

	std::vector<unsigned int> fx(nV1, 0);
	std::vector<int> classifyFx(nV1, 0);

	for (unsigned int k = minV1Index; k <= maxV1Index; ++k) {
		unsigned int area = 0;
		for (unsigned int j = minV2Index; j <= maxV2Index; ++j) {
			for (unsigned int i = minV3Index; i <= maxV3Index; ++i) {
				area += obbSpace.at(i).at(j).at(k);
			}
		}
		fx.at(k - minV1Index) = area;
	}

	// handle boundaries, if we have something like 1, 1, 1, 1, 5 - then the 4th 1 should be marked as a local minimum
	// if we have something like 0, 0, 0, 1, 5 - then we trim the 0's and the 1 is NOT marked as a local minimum
	// if we have something like 0, 0, 0, 1, 1, 1, 1, 5 - then we trim the 0's and get same situation as 1, 1, 1, 1, 5 and treat 4th 1 as a local minimum

	int startIndex = -1;
	int endIndex = -1;

	// trim any leading 0's...
	for (unsigned int i = minV1Index; i <= maxV1Index; ++i) {
		if (fx.at(i - minV1Index) > 0) {
			startIndex = i - minV1Index;
			break;
		}
	}
	// trim any trailing 0's...
	for (unsigned int i = maxV1Index; i >= minV1Index; --i) {
		if (fx.at(i - minV1Index) > 0) {
			endIndex = i - minV1Index;
			break;
		}
	}

	// TERMINATE...
	if (-1 == startIndex || -1 == endIndex || startIndex == endIndex) return -1;


	unsigned int localMinCount = 0;
	unsigned int minArea = std::numeric_limits<unsigned int>::max();
	unsigned int maxArea = 0;

	// handle boundaries (can't be marked as local minima)...
	if (fx.at(startIndex) > fx.at(startIndex + 1)) {
		classifyFx.at(startIndex) = 1;
		if (fx.at(startIndex) > maxArea) maxArea = fx.at(startIndex);
		if (fx.at(startIndex) < minArea) minArea = fx.at(startIndex);
	}
	if (fx.at(endIndex) > fx.at(endIndex - 1)) {
		classifyFx.at(endIndex) = 1;
		if (fx.at(endIndex) > maxArea) maxArea = fx.at(endIndex);
		if (fx.at(endIndex) < minArea) minArea = fx.at(endIndex);
	}
	for (unsigned int i = startIndex + 1; i < endIndex; ++i) {
		//MAXIMUM...
		if (fx.at(i - 1) < fx.at(i) && fx.at(i) >= fx.at(i + 1)) {
			classifyFx.at(i) = 1;
			if (fx.at(i) > maxArea) maxArea = fx.at(i);
			if (fx.at(i) < minArea) minArea = fx.at(i);
		}
		else if (fx.at(i - 1) <= fx.at(i) && fx.at(i) > fx.at(i + 1)) {
			classifyFx.at(i) = 1;
			if (fx.at(i) > maxArea) maxArea = fx.at(i);
			if (fx.at(i) < minArea) minArea = fx.at(i);
		}
		//MINIMUM...
		//TODO: pass in this "10" as a user parameter
		else if (i >= startIndex + 10 && i <= endIndex - 10) {
			if (fx.at(i - 1) > fx.at(i) && fx.at(i) <= fx.at(i + 1)) {
				classifyFx.at(i) = -1;
				if (fx.at(i) > maxArea) maxArea = fx.at(i);
				if (fx.at(i) < minArea) minArea = fx.at(i);
				++localMinCount;
			}
			else if (fx.at(i - 1) >= fx.at(i) && fx.at(i) < fx.at(i + 1)) {
				classifyFx.at(i) = -1;
				if (fx.at(i) > maxArea) maxArea = fx.at(i);
				if (fx.at(i) < minArea) minArea = fx.at(i);
				++localMinCount;
			}
		}
	}

	//TODO: could add in t1/t2 termination conditions here (or just handle t1 here and then handle t2 before even getting here by caller)
	// TERMINATE...
	if (0 == localMinCount) return -1;


	//TODO: ignore local minima with more than 1 part in cross-section
	// find the biggest JUMP...
	int spliceIndex = -1;
	float maxDYDX = std::numeric_limits<float>::lowest();

	for (unsigned int i = startIndex; i <= endIndex; ++i) {
		// if at a minimum...
		if (-1 == classifyFx.at(i)) {

			// find slope between this minimum and every other maximum...
			for (unsigned int j = startIndex; j <= endIndex; ++j) {
				if (1 == classifyFx.at(j)) {
					//NOTE: if the minimum is higher up than maximum, the slope will be negative/0 and thus be too small to even consider
					int const dx = glm::abs<int>((int)j - (int)i);
					int const dy = fx.at(j) - fx.at(i);
					float const dydx = ((float)dy) / dx;

					//TODO: could handle ties differently
					if (dydx > maxDYDX) {
						maxDYDX = dydx;
						spliceIndex = minV1Index + i;
					}
				}
			}
		}
	}

	return spliceIndex;
}


//NOTE: RETURN -1 on no index found
int Program::searchForSpliceIndexOverV2(std::vector<std::vector<std::vector<unsigned int>>> const& obbSpace, unsigned int const minV1Index, unsigned int const maxV1Index, unsigned int const minV2Index, unsigned int const maxV2Index, unsigned int const minV3Index, unsigned int const maxV3Index) {

	unsigned int const nV1 = (maxV1Index - minV1Index) + 1;
	unsigned int const nV2 = (maxV2Index - minV2Index) + 1;
	unsigned int const nV3 = (maxV3Index - minV3Index) + 1;

	std::vector<unsigned int> fx(nV2, 0);
	std::vector<int> classifyFx(nV2, 0);

	for (unsigned int j = minV2Index; j <= maxV2Index; ++j) {
		unsigned int area = 0;
		for (unsigned int i = minV3Index; i <= maxV3Index; ++i) {
			for (unsigned int k = minV1Index; k <= maxV1Index; ++k) {
				area += obbSpace.at(i).at(j).at(k);
			}
		}
		fx.at(j - minV2Index) = area;
	}

	// handle boundaries, if we have something like 1, 1, 1, 1, 5 - then the 4th 1 should be marked as a local minimum
	// if we have something like 0, 0, 0, 1, 5 - then we trim the 0's and the 1 is NOT marked as a local minimum
	// if we have something like 0, 0, 0, 1, 1, 1, 1, 5 - then we trim the 0's and get same situation as 1, 1, 1, 1, 5 and treat 4th 1 as a local minimum

	int startIndex = -1;
	int endIndex = -1;

	// trim any leading 0's...
	for (unsigned int i = minV2Index; i <= maxV2Index; ++i) {
		if (fx.at(i - minV2Index) > 0) {
			startIndex = i - minV2Index;
			break;
		}
	}
	// trim any trailing 0's...
	for (unsigned int i = maxV2Index; i >= minV2Index; --i) {
		if (fx.at(i - minV2Index) > 0) {
			endIndex = i - minV2Index;
			break;
		}
	}

	// TERMINATE...
	if (-1 == startIndex || -1 == endIndex || startIndex == endIndex) return -1;


	unsigned int localMinCount = 0;
	unsigned int minArea = std::numeric_limits<unsigned int>::max();
	unsigned int maxArea = 0;

	// handle boundaries (can't be marked as local minima)...
	if (fx.at(startIndex) > fx.at(startIndex + 1)) {
		classifyFx.at(startIndex) = 1;
		if (fx.at(startIndex) > maxArea) maxArea = fx.at(startIndex);
		if (fx.at(startIndex) < minArea) minArea = fx.at(startIndex);
	}
	if (fx.at(endIndex) > fx.at(endIndex - 1)) {
		classifyFx.at(endIndex) = 1;
		if (fx.at(endIndex) > maxArea) maxArea = fx.at(endIndex);
		if (fx.at(endIndex) < minArea) minArea = fx.at(endIndex);
	}
	for (unsigned int i = startIndex + 1; i < endIndex; ++i) {
		//MAXIMUM...
		if (fx.at(i - 1) < fx.at(i) && fx.at(i) >= fx.at(i + 1)) {
			classifyFx.at(i) = 1;
			if (fx.at(i) > maxArea) maxArea = fx.at(i);
			if (fx.at(i) < minArea) minArea = fx.at(i);
		}
		else if (fx.at(i - 1) <= fx.at(i) && fx.at(i) > fx.at(i + 1)) {
			classifyFx.at(i) = 1;
			if (fx.at(i) > maxArea) maxArea = fx.at(i);
			if (fx.at(i) < minArea) minArea = fx.at(i);
		}
		//MINIMUM...
		//TODO: pass in this "10" as a user parameter
		else if (i >= startIndex + 10 && i <= endIndex - 10) {
			if (fx.at(i - 1) > fx.at(i) && fx.at(i) <= fx.at(i + 1)) {
				classifyFx.at(i) = -1;
				if (fx.at(i) > maxArea) maxArea = fx.at(i);
				if (fx.at(i) < minArea) minArea = fx.at(i);
				++localMinCount;
			}
			else if (fx.at(i - 1) >= fx.at(i) && fx.at(i) < fx.at(i + 1)) {
				classifyFx.at(i) = -1;
				if (fx.at(i) > maxArea) maxArea = fx.at(i);
				if (fx.at(i) < minArea) minArea = fx.at(i);
				++localMinCount;
			}
		}
	}

	//TODO: could add in t1/t2 termination conditions here (or just handle t1 here and then handle t2 before even getting here by caller)
	// TERMINATE...
	if (0 == localMinCount) return -1;


	//TODO: ignore local minima with more than 1 part in cross-section
	// find the biggest JUMP...
	int spliceIndex = -1;
	float maxDYDX = std::numeric_limits<float>::lowest();

	for (unsigned int i = startIndex; i <= endIndex; ++i) {
		// if at a minimum...
		if (-1 == classifyFx.at(i)) {

			// find slope between this minimum and every other maximum...
			for (unsigned int j = startIndex; j <= endIndex; ++j) {
				if (1 == classifyFx.at(j)) {
					//NOTE: if the minimum is higher up than maximum, the slope will be negative/0 and thus be too small to even consider
					int const dx = glm::abs<int>((int)j - (int)i);
					int const dy = fx.at(j) - fx.at(i);
					float const dydx = ((float)dy) / dx;

					//TODO: could handle ties differently
					if (dydx > maxDYDX) {
						maxDYDX = dydx;
						spliceIndex = minV2Index + i;
					}
				}
			}
		}
	}

	return spliceIndex;
}


//NOTE: RETURN -1 on no index found
int Program::searchForSpliceIndexOverV3(std::vector<std::vector<std::vector<unsigned int>>> const& obbSpace, unsigned int const minV1Index, unsigned int const maxV1Index, unsigned int const minV2Index, unsigned int const maxV2Index, unsigned int const minV3Index, unsigned int const maxV3Index) {

	unsigned int const nV1 = (maxV1Index - minV1Index) + 1;
	unsigned int const nV2 = (maxV2Index - minV2Index) + 1;
	unsigned int const nV3 = (maxV3Index - minV3Index) + 1;

	std::vector<unsigned int> fx(nV3, 0);
	std::vector<int> classifyFx(nV3, 0);

	for (unsigned int i = minV3Index; i <= maxV3Index; ++i) {
		unsigned int area = 0;
		for (unsigned int j = minV2Index; j <= maxV2Index; ++j) {
			for (unsigned int k = minV1Index; k <= maxV1Index; ++k) {
				area += obbSpace.at(i).at(j).at(k);
			}
		}
		fx.at(i - minV3Index) = area;
	}

	// handle boundaries, if we have something like 1, 1, 1, 1, 5 - then the 4th 1 should be marked as a local minimum
	// if we have something like 0, 0, 0, 1, 5 - then we trim the 0's and the 1 is NOT marked as a local minimum
	// if we have something like 0, 0, 0, 1, 1, 1, 1, 5 - then we trim the 0's and get same situation as 1, 1, 1, 1, 5 and treat 4th 1 as a local minimum

	int startIndex = -1;
	int endIndex = -1;

	// trim any leading 0's...
	for (unsigned int i = minV3Index; i <= maxV3Index; ++i) {
		if (fx.at(i - minV3Index) > 0) {
			startIndex = i - minV3Index;
			break;
		}
	}
	// trim any trailing 0's...
	for (unsigned int i = maxV3Index; i >= minV3Index; --i) {
		if (fx.at(i - minV3Index) > 0) {
			endIndex = i - minV3Index;
			break;
		}
	}

	// TERMINATE...
	if (-1 == startIndex || -1 == endIndex || startIndex == endIndex) return -1;


	unsigned int localMinCount = 0;
	unsigned int minArea = std::numeric_limits<unsigned int>::max();
	unsigned int maxArea = 0;

	// handle boundaries (can't be marked as local minima)...
	if (fx.at(startIndex) > fx.at(startIndex + 1)) {
		classifyFx.at(startIndex) = 1;
		if (fx.at(startIndex) > maxArea) maxArea = fx.at(startIndex);
		if (fx.at(startIndex) < minArea) minArea = fx.at(startIndex);
	}
	if (fx.at(endIndex) > fx.at(endIndex - 1)) {
		classifyFx.at(endIndex) = 1;
		if (fx.at(endIndex) > maxArea) maxArea = fx.at(endIndex);
		if (fx.at(endIndex) < minArea) minArea = fx.at(endIndex);
	}
	for (unsigned int i = startIndex + 1; i < endIndex; ++i) {
		//MAXIMUM...
		if (fx.at(i - 1) < fx.at(i) && fx.at(i) >= fx.at(i + 1)) {
			classifyFx.at(i) = 1;
			if (fx.at(i) > maxArea) maxArea = fx.at(i);
			if (fx.at(i) < minArea) minArea = fx.at(i);
		}
		else if (fx.at(i - 1) <= fx.at(i) && fx.at(i) > fx.at(i + 1)) {
			classifyFx.at(i) = 1;
			if (fx.at(i) > maxArea) maxArea = fx.at(i);
			if (fx.at(i) < minArea) minArea = fx.at(i);
		}
		//MINIMUM...
		//TODO: pass in this "10" as a user parameter
		else if (i >= startIndex + 10 && i <= endIndex - 10) {
			if (fx.at(i - 1) > fx.at(i) && fx.at(i) <= fx.at(i + 1)) {
				classifyFx.at(i) = -1;
				if (fx.at(i) > maxArea) maxArea = fx.at(i);
				if (fx.at(i) < minArea) minArea = fx.at(i);
				++localMinCount;
			}
			else if (fx.at(i - 1) >= fx.at(i) && fx.at(i) < fx.at(i + 1)) {
				classifyFx.at(i) = -1;
				if (fx.at(i) > maxArea) maxArea = fx.at(i);
				if (fx.at(i) < minArea) minArea = fx.at(i);
				++localMinCount;
			}
		}
	}

	//TODO: could add in t1/t2 termination conditions here (or just handle t1 here and then handle t2 before even getting here by caller)
	// TERMINATE...
	if (0 == localMinCount) return -1;


	//TODO: ignore local minima with more than 1 part in cross-section
	// find the biggest JUMP...
	int spliceIndex = -1;
	float maxDYDX = std::numeric_limits<float>::lowest();

	for (unsigned int i = startIndex; i <= endIndex; ++i) {
		// if at a minimum...
		if (-1 == classifyFx.at(i)) {

			// find slope between this minimum and every other maximum...
			for (unsigned int j = startIndex; j <= endIndex; ++j) {
				if (1 == classifyFx.at(j)) {
					//NOTE: if the minimum is higher up than maximum, the slope will be negative/0 and thus be too small to even consider
					int const dx = glm::abs<int>((int)j - (int)i);
					int const dy = fx.at(j) - fx.at(i);
					float const dydx = ((float)dy) / dx;

					//TODO: could handle ties differently
					if (dydx > maxDYDX) {
						maxDYDX = dydx;
						spliceIndex = minV3Index + i;
					}
				}
			}
		}
	}

	return spliceIndex;
}
