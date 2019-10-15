#version 430 core

uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPos;

void main(void) {
	gl_Position = projection * view * vec4(lightPos, 1.0);        
}
