#version 430 core

uniform mat4 mvp;

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 colour;

out vec3 COLOUR;

void main(void) {
	gl_Position = mvp * vec4(vertex, 1.0f);
	COLOUR = colour;
}