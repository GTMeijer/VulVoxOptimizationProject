#version 450

layout(set = 0, binding = 0) uniform MVP
{
	mat4 model;
	mat4 view;
	mat4 projection;
} mvp;

//Vertex attributes
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_texture_coordinate;

//Instance attributes
layout(location = 3) in mat4 instance_model_matrix; //mat4 uses 4 slots
layout(location = 7) in uint instance_texture_index; 

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec3 frag_texture_coordinate;

void main()
{
	frag_color = in_color;
	
	frag_texture_coordinate = vec3(in_texture_coordinate, instance_texture_index);

	//gl_InstanceIndex
	gl_Position = mvp.projection * mvp.view * mvp.model * instance_model_matrix * vec4(in_position, 1.0);
}