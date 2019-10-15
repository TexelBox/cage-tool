#include <cmath>
#include <iostream>
#include "Texture.h"

GLuint Texture::create1DTexture(std::vector<GLubyte>& rgbaValues) {
    GLuint textureID;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_1D, textureID);
    glTexStorage1D(GL_TEXTURE_1D, 1, GL_RGBA8, rgbaValues.size()*sizeof (GLubyte) / 4);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, rgbaValues.size()*sizeof (GLubyte) / 4, GL_RGBA, GL_UNSIGNED_BYTE, rgbaValues.data());
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_1D, 0);

    return textureID;
}

GLuint Texture::create2DTexture(std::vector<unsigned char>& image, unsigned int width, unsigned int height) {
	GLuint textureID;

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return textureID;
}

void Texture::bind2DTexture(GLuint _program, GLuint _textureID, std::string varName) {
	glActiveTexture(GL_TEXTURE0 + _textureID);
	glBindTexture(GL_TEXTURE_2D, _textureID);
	glUniform1i(glGetUniformLocation(_program, varName.c_str()), _textureID);
}

void Texture::bind1DTexture(GLuint _program, GLuint _textureID, std::string varName) {
	glActiveTexture(GL_TEXTURE0 + _textureID);
	glBindTexture(GL_TEXTURE_1D, _textureID);
	glUniform1i(glGetUniformLocation(_program, varName.c_str()), _textureID);
}

void Texture::unbind1DTexture() {
	glBindTexture(GL_TEXTURE_1D, 0);
}

void Texture::unbind2DTexture() {
	glBindTexture(GL_TEXTURE_2D, 0);
}
