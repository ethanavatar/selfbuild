#version 460 core
layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 uv_color;
layout (location = 2) in vec2 texture_coordinate;

out vec3 frag_uv_color;
out vec2 frag_texture_coordinate;

void main() {
   gl_Position = vec4(vertex_position, 1.0);
   frag_uv_color = uv_color;
   frag_texture_coordinate = texture_coordinate;
}
