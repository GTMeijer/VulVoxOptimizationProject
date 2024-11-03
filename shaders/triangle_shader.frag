#version 450

layout(set = 1, binding = 1) uniform sampler2D texture_sampler;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_texture_coordinate;

layout(location = 0) out vec4 out_color;

void main()
{
	//out_color = vec4(frag_texture_coordinate, 0.0, 1.0);
	//out_color = vec4(frag_color * texture(texture_sampler, frag_texture_coordinate).rgb, 1.0);

	out_color = texture(texture_sampler, frag_texture_coordinate);
}