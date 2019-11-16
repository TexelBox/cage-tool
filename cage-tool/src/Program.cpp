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

	m_model = ObjectLoader::createTriMeshObject("models/armadillo.obj", false, true); // force ignore normals (TODO: generate them ourselves)
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

		if (ImGui::Button("COMPUTE CAGE WEIGHTS")) computeCageWeights();
		ImGui::Separator();

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
// https://math.stackexchange.com/questions/9819/area-of-a-spherical-triangle
// https://www.quora.com/What-is-the-surface-area-of-a-spherical-triangle-of-which-I-know-the-lenghths-of-arcs-which-form-the-triangle-and-the-radius-of-the-sphere
// http://mathworld.wolfram.com/SphericalTriangle.html
// http://mathworld.wolfram.com/SphericalTrigonometry.html
//NOTE: there might be a bug here cause the 2nd tap of the button adds extra memory for some reason (its like 3MB extra)
//TODO: safety checking might have to be added, or otherwise make sure the calculation still works if cage face is less than 1 unit from model (does the unit sphere need to be shrunk more?)
//TODO: i did fix a bug where some weights were -nan due to the sqrt(<0) since I used the wrong formula for s (i forgot to divide by 2). so, more testing needs to be done to ensure this doesnt happen again
//NOTE: right now, idk how this behaves with per-face normals, I would like if the models were only per-vertex normals (thus every vertex is unique)
//TODO: implement HC/GC weights
// this method computes the vector of cage vertex weights for every model vertex, in the current orientation of both meshes
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

		// process...
		// 1. foreach model vert, compute the vector of cage vert weights
		// - assume we have model vert i and cage face f,
		// - project a unit sphere centered on i
		// - project the triangular face f onto the surface of the unit sphere
		// - calculate the spherical triangle area and save that as partial weight for points 1,2,3 making up f

		//TODO: make sure the indexing makes sense
		//TODO?: change the notation later
		//TODO: edge cage error checking
		for (unsigned int i = 0; i < m_model->drawVerts.size(); ++i) {

			// init the cage vert weights vector for model vert i...
			std::vector<float> u_i(m_cage->drawVerts.size(), 0.0f);

			//NOTE: since we are using a unit sphere (R=1), the radius will be implicit in all calculations
			glm::vec3 const sphereOrigin = m_model->drawVerts.at(i);

			// foreach triangle face in cage mesh...
			for (unsigned int f = 0; f < m_cage->drawFaces.size(); f += 3) {

				// save the 3 cage vert indices making up cage face f...
				unsigned int const a_index = m_cage->drawFaces.at(f);
				unsigned int const b_index = m_cage->drawFaces.at(f+1);
				unsigned int const c_index = m_cage->drawFaces.at(f+2);

				// get the 3 cage vert positions...
				glm::vec3 const a_pos = m_cage->drawVerts.at(a_index);
				glm::vec3 const b_pos = m_cage->drawVerts.at(b_index);
				glm::vec3 const c_pos = m_cage->drawVerts.at(c_index);

				// find the projected 3 points on unit sphere to make up spherical triangle whose area we need
				// I find the intersection vector from sphere origin directed towards triangle point (not other direction, in case the triangle happens to be within sphere somehow)
				// thus we will only ever have exactly 1 intersection point with a positive scalar value of R=1

				// all 3 of these positions will be relative to the sphere origin (thus all lengths should be R=1)
				glm::vec3 const a_proj_pos = glm::normalize(a_pos - sphereOrigin);
				glm::vec3 const b_proj_pos = glm::normalize(b_pos - sphereOrigin);
				glm::vec3 const c_proj_pos = glm::normalize(c_pos - sphereOrigin);

				// apply formula...
				// explanation...
				// 1. a is the angle between b_proj_pos (origin to spherical tri point b) and c_proj_pos (origin to spherical tri point c)
				// we know the formula for the dot product as... b_proj_pos dot c_proj_pos = ||b_proj_pos|| * ||c_proj_pos|| * cos(a)
				// rearrange the formula to get the angle a, noting that both lengths are R=1 and factored out
				// 2. next, we need to get the corresponding arc length of 1 side of the spherical triangle
				// the general formula for an angle theta in radians is of course, ARCLENGTH = R*theta
				// but, since R=1, ARCLENGTH = theta, thus we can just use the angles a,b,c as the arclengths
				// 3. next, we need to calculate s which is the SEMI-PERIMETER of the spherical triangle arclengths (so perimeter = a+b+c) divided by 2

				float const a = glm::acos(glm::dot(b_proj_pos, c_proj_pos));
				float const b = glm::acos(glm::dot(c_proj_pos, a_proj_pos));
				float const c = glm::acos(glm::dot(a_proj_pos, b_proj_pos));
				float const s = (a + b + c) / 2;

				//TODO: must make sure that we arent taking a sqrt of a negative number ever and result with a -nan (this was happening since 1 tangent was negative) before i changed s to now div by 2
				//NOTE: area should always be >=0 of course, thus it should follow that all weights will be >=0
				//TODO?: should I clamp the weights above 0 just to be safe? - there could be some numerical error in floats, for really small weights
				// 4. calculate the area of the spherical triangle from this formula based on the side lengths (arcs)
				// this formula is to actually calculate the spherical excess E = 4 * ...
				// but, since the area of the spherical triangle is given by AREA = E * R^2 (and R=1), so AREA = E = 4 * ...
				float const area = 4 * glm::atan(glm::sqrt(glm::tan(s / 2) * glm::tan((s - a) / 2) * glm::tan((s - b) / 2) * glm::tan((s - c) / 2)));

				//NOTE: I don't believe there's a need to avg by number of faces that vert is appart of. A vert should have its weight set based on the sum of all face areas its connected to since these faces get modified when this vert gets modified
				// 5. each vert gets this extra weight
				u_i.at(a_index) += area;
				u_i.at(b_index) += area;
				u_i.at(c_index) += area;
			}

			// 6. normalize the weights vector (sum of all elements = 1) for affine property

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


//TODO?: add a check to see if weights exist at the moment (i.e. were they properly computed previously and do the weights match the current coord system)
//TODO: testing
void Program::deformModel() {
	if (nullptr == m_model || nullptr == m_cage) return;

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

	//TODO:
	//0. all new drawVerts were just calculated and set in the vector
	//1. recalculate any necessary geometry changes for new deformed model such as the normals (per vertex normal calculation - the angle way?)
	//2. update buffers

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

