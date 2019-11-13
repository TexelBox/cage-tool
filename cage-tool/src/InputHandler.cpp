#include "InputHandler.h"
#include "Program.h"

std::shared_ptr<RenderEngine> InputHandler::renderEngine = nullptr;
std::shared_ptr<Camera> InputHandler::camera = nullptr;
int InputHandler::mouseOldX;
int InputHandler::mouseOldY;

// Must be called before processing any GLFW events
void InputHandler::setUp(std::shared_ptr<RenderEngine> renderEngine, std::shared_ptr<Camera> camera) {
	InputHandler::renderEngine = renderEngine;
	InputHandler::camera = camera;
}

// Callback for key presses
void InputHandler::key(GLFWwindow *window, int key, int scancode, int action, int mods) {

	Program *program = (Program*)glfwGetWindowUserPointer(window);

	if (GLFW_PRESS == action || GLFW_REPEAT == action) { // key press, or press & hold
		
		//TODO: call prorgam's translateSelectedVerts(), passing in the proper direction for each
		if (GLFW_KEY_W == key) {
			//renderEngine->updateLightPos(glm::vec3(0.0, 0.1, 0.0));
		} else if (GLFW_KEY_S == key) {
			//renderEngine->updateLightPos(glm::vec3(0.0, -0.1, 0.0));
		} else if (GLFW_KEY_A == key) {
			//renderEngine->updateLightPos(glm::vec3(-0.1, 0.0, 0.0));
		} else if (GLFW_KEY_D == key) {
			//renderEngine->updateLightPos(glm::vec3(0.1, 0.0, 0.0));
		} else if (GLFW_KEY_E == key) {
			//renderEngine->updateLightPos(glm::vec3(0.0, 0.0, 0.1));
		} else if (GLFW_KEY_Q == key) {
			//renderEngine->updateLightPos(glm::vec3(0.0, 0.0, -0.1));
		} else if (GLFW_KEY_ESCAPE == key) glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

// Callback for mouse button presses
void InputHandler::mouse(GLFWwindow *window, int button, int action, int mods) {

	Program *program = (Program*)glfwGetWindowUserPointer(window);

	if (GLFW_MOUSE_BUTTON_RIGHT == button && GLFW_PRESS == action && nullptr != program->m_model && nullptr != program->m_cage) {

		// COLOUR PICKING (back buffer querying)...
		// reference: http://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
		// reference: https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_slow_easy.cpp

		// save...
		PolygonMode const prevCagePolygonMode = program->m_cage->m_polygonMode; // should be LINE
		ColourMode const prevCageColourMode = program->m_cage->m_colourMode; // should be NORMAL
		
		// update...
		program->m_cage->m_polygonMode = PolygonMode::POINT; // must render cage as points for picking
		// update cage colour buffer to use pickingColours
		program->m_cage->m_colourMode = ColourMode::PICKING;
		renderEngine->updateBuffers(*(program->m_cage), false, false, false, true);

		// render...
		renderEngine->renderPicking({program->m_model, program->m_cage});
		
		// reset...
		program->m_cage->m_polygonMode = prevCagePolygonMode;
		program->m_cage->m_colourMode = prevCageColourMode;
		// update cage colour buffer to use original colours (should be the normal colours vector)
		renderEngine->updateBuffers(*(program->m_cage), false, false, false, true);


		// PROCESS DATA...
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		int width, height;
		glfwGetWindowSize(window, &width, &height);

		ypos = (height - 1) - ypos; // flip the y-coord (readpixels expects the +y axis to point upwards, rather than downwards like window)

		glFlush();
		glFinish();

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		unsigned char rgba[4] = {0, 0, 0, 0};
		glReadPixels(xpos, ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

		unsigned int const pickedID = rgba[0] + (rgba[1] << 8) + (rgba[2] << 16);


		// only handle picked colours belonging to cage vert picking range...
		if (pickedID < program->m_cage->pickingColours.size()) {
			//hack for now...
			program->m_cage->colours.at(pickedID) = glm::vec3(1.0f, 1.0f, 0.0f);
			renderEngine->updateBuffers(*(program->m_cage), false, false, false, true);
			//TODO: call program method that takes in this ID and toggles the picking state of the unique cage vertex this belongs to (MUST MAKE SURE CAGES ONLY HAVE UNIQUE VERTS, since they don't have uvs, or normals?)
		}

		//TODO: have an imgui button that clears all selected verts, another imgui that selects all (this would be nice if the user wants to work with alot of verts at once)
		//TODO: render a selected vert as a different render color (but picking color must stay same)
		//TODO: modify above method for key-control to no longer move light source, but instead move all selected verts in 6 axes (+ve and -ve)
		//TODO: can i modify code to make it so light source is always at camera eye, or maybe directed along vector from origin to camera eye,. but actually 500 units away from origin (to make illumination less harsh?)
		//TODO: ignore imported normals (if any), then generate per vertex normals using angular-weights?
		// can also generate per-face normals to use in calculations (optional thing that we should only do when needed)
		//TODO?: calculate the extents of the imported model and scale down for consistency
		//TODO: should attach which shader program to use to render each object (e.g. the 3 planes should use a trivial shader, since they don't have uvs or normals - actually this might not matter, since these attributes are never enabled through glEnableVertexAttribArray and thus are ignored by shader??? (not sure if this makes sense)
		//TODO: add an input method that is able to translate all selected verts in view plane using mouse swinging
		//TODO: add an input method that is able to translate verts along their normals? extrude/intrude faces along normals?
	}

	if (action == GLFW_PRESS) {
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		mouseOldX = x;
		mouseOldY = y;
	}
}

// Callback for mouse motion
void InputHandler::motion(GLFWwindow *window, double x, double y) {
	double dx, dy;
	dx = (x - mouseOldX);
	dy = (y - mouseOldY);

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
		camera->updateLongitudeRotation(dx * 0.5);
		camera->updateLatitudeRotation(dy * 0.5);
	}

	mouseOldX = x;
	mouseOldY = y;
}

// Callback for mouse scroll
void InputHandler::scroll(GLFWwindow *window, double x, double y) {
	double dy;
	dy = (x - y);
	//camera->updatePosition(glm::vec3(0.0, 0.0, dy * 0.1));
	camera->updatePosition(glm::vec3(0.0, 0.0, dy * 10)); // faster scrolling
}

// Callback for window reshape/resize
void InputHandler::reshape(GLFWwindow *window, int width, int height) {
	renderEngine->setWindowSize(width, height);
}
