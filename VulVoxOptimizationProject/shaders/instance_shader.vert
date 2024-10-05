#version 450

layout(binding = 0) uniform MVP
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
layout(location = 3) in vec3 instance_position;
layout(location = 4) in vec3 instance_rotation;
layout(location = 5) in float instance_scale;
layout(location = 6) in int instance_texture_index;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec3 frag_texture_coordinate;

void main()
{
	frag_color = in_color;
	
	frag_texture_coordinate = vec3(in_texture_coordinate, instance_texture_index);

	//Create rotation matrix
	mat3 mx, my, mz;

	//Rotate around x
	float s = sin(instance_rotation.x);
	float c = cos(instance_rotation.x);

	mx[0] = vec3(  c,   s, 0.0);
	mx[1] = vec3( -s,   c, 0.0);
	mx[2] = vec3(0.0, 0.0, 1.0);

	//Rotate around y
	s = sin(instance_rotation.y);
	c = cos(instance_rotation.y);

	my[0] = vec3(  c, 0.0,   s);
	my[1] = vec3(0.0, 1.0, 0.0);
	my[2] = vec3( -s, 0.0,   c);

	//Rotate around z
	s = sin(instance_rotation.z);
	c = cos(instance_rotation.z);

	mz[0] = vec3(1.0, 0.0, 0.0);
	mz[1] = vec3(0.0,   c,   s);
	mz[2] = vec3(0.0,  -s,   c);

	mat3 rotation_matrix = mz * my * mx;

	vec4 rotated_position = vec4(in_position.xyz * rotation_matrix, 1.0);
	vec4 pos = vec4((rotated_position.xyz * instance_scale) + instance_position, 1.0);
	
	gl_Position = mvp.projection * mvp.view * mvp.model * pos;
}