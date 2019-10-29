#include "Program.h"

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
void Program::createTestMeshObject() {
	m_model = ObjectLoader::createMeshObject("models/armadillo-with-normals.obj");
	if (m_model->hasTexture) m_model->textureID = renderEngine->loadTexture("textures/default.png"); // apply default texture (if there are uvs)
	m_model->setScale(glm::vec3(0.02f, 0.02f, 0.02f));
	meshObjects.push_back(m_model);
	renderEngine->assignBuffers(*m_model);

	m_cage = ObjectLoader::createMeshObject("models/armadillo_cage-with-normals.obj");
	if (m_cage->hasTexture) m_cage->textureID = renderEngine->loadTexture("textures/default.png"); // apply default texture (if there are uvs)
	m_cage->m_polygonMode = PolygonMode::LINE; // set wireframe
	m_cage->setScale(glm::vec3(0.02f, 0.02f, 0.02f));
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

		ImGui::End();
	}

}

// Main loop
void Program::mainLoop() {

	createTestMeshObject();

	// Our state
	clearColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);

	while(!glfwWindowShouldClose(window)) {
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
