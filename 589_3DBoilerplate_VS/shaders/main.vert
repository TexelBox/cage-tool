#version 430 core

uniform mat4 modelView;
uniform mat4 projection;
uniform vec3 lightPos;

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

out vec3 N;
out vec3 L;
out vec3 V;
out vec2 UV;

void main(void) {	

	UV = uv;

	// Put light in camera space
	vec4 lightCameraSpace = modelView * vec4(lightPos, 1.0);
	
	// Put normal in camera space (no non-uniform scaling so we can use just modelView)
	vec4 nCameraSpace = modelView * vec4(normal, 0.0);
	N = normalize(nCameraSpace.xyz);

	// Transform model and put in camera space
    vec4 pCameraSpace = modelView * vec4(vertex, 1.0); 
	vec3 P = pCameraSpace.xyz;
	
	// Calculate L and V vectors
	L = normalize(lightCameraSpace.xyz - P);
	V = normalize(-P);
	
    gl_Position = projection * pCameraSpace;   
}