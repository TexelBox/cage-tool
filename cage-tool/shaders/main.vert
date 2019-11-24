#version 430 core

uniform mat4 modelView;
uniform mat4 projection;
uniform vec3 lightPos;

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 colour;

out vec3 N;
out vec3 L;
out vec3 V;
out vec2 UV;
out vec3 COLOUR;

void main(void) {	

	UV = uv;

	// Put light in camera space
	vec4 lightCameraSpace = modelView * vec4(lightPos, 1.0f);
	
	// Put normal in camera space (no non-uniform scaling so we can use just modelView)
	vec4 nCameraSpace = modelView * vec4(normal, 0.0f);
	N = normalize(nCameraSpace.xyz);

	// Transform model and put in camera space
	vec4 pCameraSpace = modelView * vec4(vertex, 1.0f);
	vec3 P = pCameraSpace.xyz;
	
	// Calculate L and V vectors
	L = normalize(lightCameraSpace.xyz - P);
	V = normalize(-P);
	
	gl_Position = projection * pCameraSpace;
	
	COLOUR = colour;
}