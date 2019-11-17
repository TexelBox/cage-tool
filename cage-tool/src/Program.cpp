#include "Program.h"


// STATICS (INIT)...
glm::vec3 const Program::s_CAGE_UNSELECTED_COLOUR = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 const Program::s_CAGE_SELECTED_COLOUR = glm::vec3(1.0f, 1.0f, 0.0f);

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

// Loads an object from a .obj file. Can support textures.
//TODO: right now this initializes all our objects, but some of this initialization (model+cage) should be performed through user interaction (e.g. load obj file from disk)
void Program::createTestMeshObject() {
	
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

	// MODEL

	m_model = ObjectLoader::createTriMeshObject("models/armadillo-with-normals.obj");
	//m_model = ObjectLoader::createTriMeshObject("models/armadillo.obj", false, true); // force ignore normals (TODO: generate them ourselves)
	if (m_model->hasTexture) m_model->textureID = renderEngine->loadTexture("textures/default.png"); // apply default texture (if there are uvs)
	//m_model->setScale(glm::vec3(0.02f, 0.02f, 0.02f));
	meshObjects.push_back(m_model);
	renderEngine->assignBuffers(*m_model);

	// CAGE

	m_cage = ObjectLoader::createTriMeshObject("models/armadillo_cage.obj", true, true); // force ignore both uvs and normals if present in file
	
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

		if (ImGui::Button("TOGGLE YZ-PLANE (RED)")) m_yzPlane->m_isVisible = !m_yzPlane->m_isVisible;
		ImGui::SameLine();
		if (ImGui::Button("TOGGLE XZ-PLANE (GREEN)")) m_xzPlane->m_isVisible = !m_xzPlane->m_isVisible;
		ImGui::SameLine();
		if (ImGui::Button("TOGGLE XY-PLANE (BLUE)")) m_xyPlane->m_isVisible = !m_xyPlane->m_isVisible;
		ImGui::SameLine();
		ImGui::Text("	note: grid spacing is 10 units");

		ImGui::Separator();



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
			} else {
				if (ImGui::Button("CLEAR CAGE WEIGHTS")) m_vertWeights.clear();
			}
			ImGui::Separator();
		}

		ImGui::End();
	}

}

// Main loop
void Program::mainLoop() {

	createTestMeshObject();

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

	//TODO: call the recompute normals method on MODEL and then pass true as the updateNormals param to updateBuffers()

	renderEngine->updateBuffers(*m_model, true, false, false, false);
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
