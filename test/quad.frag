#version 460 core
out vec4 frag_color;
in  vec3 frag_uv_color;
in  vec2 frag_texture_coordinate;

uniform sampler2D frag_texture;

void main() {
   //frag_color = triangle_color;
   //frag_color = vec4(frag_uv_color, 1.0);
   frag_color = texture(frag_texture, frag_texture_coordinate);
}
