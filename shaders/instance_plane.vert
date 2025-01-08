#version 450

layout(set = 0, binding = 0) uniform MVP
{
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;

//Instance attributes, we skip the vertex attributes
layout(location = 3) in mat4 instance_model_matrix; //mat4 uses 4 slots
layout(location = 7) in uint instance_texture_index;
layout(location = 8) in vec2 instance_uv_min;
layout(location = 9) in vec2 instance_uv_max;


//We can re-use the instance_shader.frag, use the same outputs
layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec3 frag_texture_coordinate;

void main() {

    //hardcoded positions and texcoords for a unit square (two triangles)
    vec2 vertices[6] = vec2[]
    (
        vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(-0.5, 0.5),
        vec2( 0.5, -0.5), vec2(0.5,  0.5), vec2(-0.5, 0.5)
    );
    vec2 texcoords[6] = vec2[]
    (
        vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(0.0, 0.0),
        vec2(1.0, 1.0), vec2(1.0, 0.0), vec2(0.0, 0.0)
    );

    vec2 vertex = vertices[gl_VertexIndex];
    vec2 texcoord = texcoords[gl_VertexIndex];

    //compute position
    gl_Position = mvp.projection * mvp.view * mvp.model * instance_model_matrix * vec4(vertex, 0.0, 1.0);

    //compute texture coordinates
    //vec2 uv_min = vec2(0,0);
    //vec2 uv_max = vec2(1,1);
    //vec2 uv = mix(uv_min, uv_max, texcoord);
    vec2 uv = mix(instance_uv_min, instance_uv_max, texcoord);
    frag_texture_coordinate = vec3(uv, float(instance_texture_index));
    //frag_texture_coordinate = vec3(mix(instance_uv_min, instance_uv_max, texcoord), instance_texture_index);


    frag_color = vec3(1,1,1);
}