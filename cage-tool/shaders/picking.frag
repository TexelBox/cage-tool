#version 430 core

uniform bool hasPickingColour;

in vec3 COLOUR;

out vec4 colour;

void main(void) {
	// 3 use cases (2/3 are treated the same)
	// 1. (object as whole has no picking colours => every vert gets a default white picking colour)
	// 2. (object as a whole has 1 uniform picking colour => every vert gets this picking colour (passed in buffer data will be uniform))
	// 3. (object has verts of differing picking colours => each vert gets its specified picking colour passed in through buffer)
	colour = hasPickingColour ? vec4(COLOUR, 1.0f) : vec4(1.0f, 1.0f, 1.0f, 1.0f);
}