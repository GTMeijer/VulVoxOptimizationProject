#version 450

layout(binding = 1) uniform sampler2D texture_sampler_array;
//layout(binding = 1) uniform sampler2DArray texture_sampler_array;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec3 frag_texture_coordinate;

layout(location = 0) out vec4 out_color;

void main()
{
	//out_color = texture(texture_sampler_array, frag_texture_coordinate);
	out_color = vec4(frag_color * texture(texture_sampler_array, frag_texture_coordinate.rg).rgb, 1.0);
}