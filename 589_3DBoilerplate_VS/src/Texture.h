#pragma once

#include "GL/glew.h"
#include <vector>
#include <string>

class Texture {

public:
	static GLuint create1DTexture(std::vector<GLubyte>& rgbaValues);
	static GLuint create2DTexture(std::vector<unsigned char>& image, unsigned int width, unsigned int height);
	 
	static void bind1DTexture(GLuint _program, GLuint _textureID, std::string varName);
	static void bind2DTexture(GLuint _program, GLuint _textureID, std::string varName);
	 
	static void unbind1DTexture();
	static void unbind2DTexture();
};
