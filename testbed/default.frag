#version 460 core
out vec4 final_color;
in  vec4 frag_color;
in  vec2 frag_texture_coordinate;

uniform sampler2D texture0;

void main() {
   vec4 texel_color = texture(texture0, frag_texture_coordinate);
   final_color = texel_color * frag_color;
}
