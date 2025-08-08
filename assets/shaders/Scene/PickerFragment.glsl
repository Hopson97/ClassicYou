#version 460

// The picker texture is just a single channel (red, GL_R32I) texture.
// When the 3d view is right-clicked, each object's ID is written to the texture channel such that
// the object ID can read-back via glReadPixels
layout(location = 0) out int out_colour;

uniform int object_id;

void main() {
    out_colour = object_id;
}
