#version 460 core
out vec4 frag_color;
in  vec2 frag_texture_coordinate;

uniform sampler2D frag_texture;

void main() {
   frag_color = texture(frag_texture, frag_texture_coordinate);
}
