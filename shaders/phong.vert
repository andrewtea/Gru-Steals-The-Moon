#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform vec3 camera_position;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 texcoords;
	vec3 tangent;
	vec3 binormal;
} vs_out;


void main()
{
	vs_out.vertex = camera_position - vec3(vertex_model_to_world * vec4(vertex, 1.0));
	vs_out.normal = normal;	
	vs_out.texcoords = texcoords.xy;
	vs_out.tangent = tangent;
	vs_out.binormal = binormal;

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}



