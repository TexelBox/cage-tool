#pragma once

#include <GL/glew.h>
#include <iostream>
#include <fstream>

// Class modified from code provided by Allan Rocha for CPSC 591
class ShaderTools {

public:
	static GLuint compileShaders(const char* vertexFilename, const char* fragmentFilename);
	static GLuint compileShaders(const char* vertexFilename, const char* geometryFilename, const char* fragmentFilename);

private:
	static unsigned long getFileLength(std::ifstream& file);
	static GLchar* loadshader(std::string filename);
	static void unloadshader( GLchar** ShaderSource );
};
