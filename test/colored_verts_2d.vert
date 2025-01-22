#version 460 core
layout (location = 0) in vec3 a_vertex_position;
layout (location = 1) in vec3 a_vertex_color;

out vec3 vert_color;

uniform mat4 transform;
uniform mat4 projection;

void main() {
   gl_Position = projection * transform * vec4(a_vertex_position, 1.0f);
   vert_color = a_vertex_color;
}
