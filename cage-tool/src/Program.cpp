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
	std::shared_ptr<MeshObject> o = ObjectLoader::createMeshObject("models/Yoshi.obj");
	if (o->hasTexture) o->textureID = renderEngine->loadTexture("textures/default.png"); // assign default texture to a mesh with uvs (can later on ask user for real texture if they have one)
	meshObjects.push_back(o);
	renderEngine->assignBuffers(*o);
}

void Program::drawUI() {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		ImGui::Begin("EDIT WINDOW");                          // Create a window called "Hello, world!" and append into it.

		ImGui::ColorEdit3("clear color", (float*)&clearColor); // Edit 3 floats representing a color

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
