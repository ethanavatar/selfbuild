#version 460 core
layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec2 texture_coordinate;

out vec2 frag_texture_coordinate;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
   gl_Position = projection * view * model * vec4(vertex_position, 1.0f);
   frag_texture_coordinate = texture_coordinate;
}
