#version 430 core

in vec3 COLOUR;

out vec4 colour;

void main(void) {
	colour = vec4(COLOUR, 1.0f);
}