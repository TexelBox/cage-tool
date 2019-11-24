#version 430 core

uniform sampler2D image;
uniform bool hasTexture;
uniform bool hasNormals;

in vec3 N;
in vec3 L;
in vec3 V;
in vec2 UV;
in vec3 COLOUR;

out vec4 colour;

void main(void) {    	

	/***** Image-based texturing *****/
	vec4 imgColour;

	if (hasTexture) {
		imgColour = texture(image, UV);
	} else {
		imgColour = vec4(COLOUR, 1.0f);
	}

	if (hasNormals) {
		float diffuse =  (dot(N, L) + 1) / 2;
		vec3 diffuseColour = diffuse * vec3(imgColour.x, imgColour.y, imgColour.z);

		colour = vec4(diffuseColour, imgColour.w);
	} else {
		colour = imgColour;
	}
}