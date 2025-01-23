#version 460 core
layout (location = 0) in vec3 a_vertex_position;
layout (location = 1) in vec4 a_vertex_color;
layout (location = 2) in vec2 a_texture_coordinate;

out vec4 frag_color;
out vec2 frag_texture_coordinate;

uniform mat4 mvp;

void main() {
   gl_Position = mvp * vec4(a_vertex_position, 1.0f);
   frag_color = a_vertex_color;
   frag_texture_coordinate = a_texture_coordinate;
}
