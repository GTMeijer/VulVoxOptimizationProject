#version 450

layout(set = 0, binding = 0) uniform MVP
{
	mat4 model;
	mat4 view;
	mat4 projection;
} mvp;

layout(push_constant) uniform Object_Constants
{
	mat4 model;
} object_constants;

//Vertex attributes
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_texture_coordinate;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_texture_coordinate;

void main()
{
	gl_Position = mvp.projection * mvp.view * mvp.model * object_constants.model * vec4(in_position, 1.0);
	frag_color = in_color;
	frag_texture_coordinate = in_texture_coordinate;
}