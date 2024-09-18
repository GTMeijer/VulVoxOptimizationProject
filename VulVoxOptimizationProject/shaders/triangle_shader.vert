#version 450

layout(binding = 0) uniform MVP
{
	mat4 model;
	mat4 view;
	mat4 projection;
} mvp;


layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 frag_Color;

void main()
{
	gl_Position = mvp.projection * mvp.view * mvp.model * vec4(inPosition, 0.0, 1.0);
	frag_Color = inColor;
}