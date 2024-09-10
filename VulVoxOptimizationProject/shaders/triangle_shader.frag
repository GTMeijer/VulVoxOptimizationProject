#version 450

layout(location = 0) in vec3 frag_Color;

layout(location = 0) out vec4 out_Color;

void main()
{
	out_Color = vec4(frag_Color, 1.0);
}