#include "ShaderTools.h"

GLuint ShaderTools::compileShaders(const char* vertexFilename, const char* fragmentFilename) {
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;

	const GLchar * vertex_shader_source [] = {loadshader(vertexFilename)};
	const GLchar * fragment_shader_source [] = {loadshader(fragmentFilename)};

	// Create and compile vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, vertex_shader_source, NULL);
	glCompileShader(vertex_shader);

	//Create and compile a fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, fragment_shader_source, NULL);
	glCompileShader(fragment_shader);


	// Create program, attach shaders to it, and link it
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	glLinkProgram(program);

	GLint status;
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
		GLint infoLogLength;
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar* strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(fragment_shader, infoLogLength, NULL, strInfoLog);

		fprintf(stderr, "Compilation error in shader fragment_shader: %s\n", strInfoLog);
		delete[] strInfoLog;

	}

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
		GLint infoLogLength;
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar* strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(vertex_shader, infoLogLength, NULL, strInfoLog);

		fprintf(stderr, "Compilation error in shader vertex_shader: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	// Delete the shaders as the program has them now
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	unloadshader((GLchar**) vertex_shader_source);
	unloadshader((GLchar**) fragment_shader_source);

	return program;
}

GLuint ShaderTools::compileShaders(const char* vertexFilename, const char* geometryFilename, const char* fragmentFilename) {
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint geometry_shader;

	GLuint program;

	const GLchar * vertex_shader_source [] = {loadshader(vertexFilename)};
	const GLchar * fragment_shader_source [] = {loadshader(fragmentFilename)};
	const GLchar * geometry_shader_source [] = {loadshader(geometryFilename)};

	// Create and compile the vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, vertex_shader_source, NULL);
	glCompileShader(vertex_shader);

	//Create and compile the fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, fragment_shader_source, NULL);
	glCompileShader(fragment_shader);

	//Create and compile the geometry shader
	geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(geometry_shader, 1, geometry_shader_source, NULL);
	glCompileShader(geometry_shader);


	// Create program, attach shaders to it, and link it
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, geometry_shader);
	glAttachShader(program, fragment_shader);

	glLinkProgram(program);

	GLint status;
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
		GLint infoLogLength;
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar* strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(fragment_shader, infoLogLength, NULL, strInfoLog);

		fprintf(stderr, "Compilation error in shader fragment_shader: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
		GLint infoLogLength;
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar* strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(vertex_shader, infoLogLength, NULL, strInfoLog);

		fprintf(stderr, "Compilation error in shader vertex_shader: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	glGetShaderiv(geometry_shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {
		GLint infoLogLength;
		glGetShaderiv(geometry_shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar* strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(geometry_shader, infoLogLength, NULL, strInfoLog);

		fprintf(stderr, "Compilation error in shader geometry_shader: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	// Delete the shaders as the program has them now
	glDeleteShader(vertex_shader);
	glDeleteShader(geometry_shader);
	glDeleteShader(fragment_shader);


	unloadshader((GLchar**) vertex_shader_source);
	unloadshader((GLchar**) geometry_shader_source);
	unloadshader((GLchar**) fragment_shader_source);

	return program;
}

unsigned long ShaderTools::getFileLength(std::ifstream& file) {
	if (!file.good()) return 0;

	file.seekg(0, std::ios::end);
	unsigned long len = file.tellg();
	file.seekg(std::ios::beg);

	return len;
}

GLchar* ShaderTools::loadshader(std::string filename) {
	std::ifstream file;
	file.open(filename.c_str(), std::ios::in); // opens as ASCII!
	if (!file) return NULL;

	unsigned long len = getFileLength(file);

	if (len == 0) return NULL; // Error: Empty File

	GLchar* ShaderSource = 0;

	ShaderSource = new char[len + 1];

	if (ShaderSource == 0) return NULL; // can't reserve memoryf

	// len isn't always strlen cause some characters are stripped in ascii read...
	// it is important to 0-terminate the real length later, len is just max possible value...
	ShaderSource[len] = 0;

	unsigned int i = 0;
	while ( file.good() ) {
		ShaderSource[i] = file.get(); // get character from file.
		if (!file.eof())
			i++;
	}

	ShaderSource[i] = 0; // 0-terminate it at the correct position

	file.close();

	return ShaderSource; // No Error
}

void ShaderTools::unloadshader( GLchar** ShaderSource ) {
	if (*ShaderSource != 0) {
		delete[] * ShaderSource;
	}
	*ShaderSource = 0;
}
