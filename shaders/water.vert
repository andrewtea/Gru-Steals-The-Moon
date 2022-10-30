#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoords;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform vec3 camera_position;
uniform float elapsed_time;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
	float x_deriv;
	float z_deriv;
	vec2 normalCoord0;
	vec2 normalCoord1;
	vec2 normalCoord2;
} vs_out;

float calcWaveX(float amp, vec2 dir, float freq, float phase, float sharp, vec2 pos, float time) {
	return 0.5 * sharp * freq * amp * pow(sin((dir.x * pos.x + dir.y * pos.y) * freq + phase * time) * 0.5 + 0.5, sharp) *
	cos((dir.x * pos.x + dir.y * pos.y) * freq + phase * time) * dir.x;
}

float calcWaveY(float amp, vec2 dir, float freq, float phase, float sharp, vec2 pos, float time) {
	return amp * pow(sin((dir.x * pos.x + dir.y * pos.y) * freq + phase * time) * 0.5 + 0.5, sharp);
}

float calcWaveZ(float amp, vec2 dir, float freq, float phase, float sharp, vec2 pos, float time) {
	return 0.5 * sharp * freq * amp * pow(sin((dir.x * pos.x + dir.y * pos.y) * freq + phase * time) * 0.5 + 0.5, sharp) *
	cos((dir.x * pos.x + dir.y * pos.y) * freq + phase * time) * dir.y;
}

void main() {
	// Calculate wave displacement
	vec3 displaced_vertex = vertex;
	//displaced_vertex.x += calcWaveX(1.0, vec2(-1, 0), 0.2, 0.5, 2.0, vertex.xz, elapsed_time);
	//displaced_vertex.x += calcWaveX(0.5, vec2(-0.7, 0.7), 0.4, 1.3, 2.0, vertex.xz, elapsed_time);
	displaced_vertex.y += calcWaveY(1.0, vec2(-1, 0), 0.2, 0.5, 2.0, vertex.xz, elapsed_time);
	displaced_vertex.y += calcWaveY(0.5, vec2(-0.7, 0.7), 0.4, 1.3, 2.0, vertex.xz, elapsed_time);
	//displaced_vertex.z += calcWaveZ(1.0, vec2(-1, 0), 0.2, 0.5, 2.0, vertex.xz, elapsed_time);
	//displaced_vertex.z += calcWaveZ(0.5, vec2(-0.7, 0.7), 0.4, 1.3, 2.0, vertex.xz, elapsed_time);

	vs_out.vertex = vec3(vertex_model_to_world * vec4(displaced_vertex, 1.0));
	vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));
	vs_out.x_deriv = calcWaveX(1.0, vec2(-1, 0), 0.2, 0.5, 2.0, vertex.xz, elapsed_time);
	vs_out.x_deriv += calcWaveX(0.5, vec2(-0.7, 0.7), 0.4, 1.3, 2.0, vertex.xz, elapsed_time);
	vs_out.z_deriv = calcWaveZ(1.0, vec2(-1, 0), 0.2, 0.5, 2.0, vertex.xz, elapsed_time);
	vs_out.z_deriv += calcWaveZ(0.5, vec2(-0.7, 0.7), 0.4, 1.3, 2.0, vertex.xz, elapsed_time);

	// Calculate normal coords
	vec2 texScale = vec2(8, 4);
	float normalTime = mod(elapsed_time, 100.0);
	vec2 normalSpeed = vec2(-0.05, 0.0);

	vs_out.normalCoord0.xy = texcoords.xz * texScale + normalTime * normalSpeed;
	vs_out.normalCoord1.xy = texcoords.xz * texScale * 2 + normalTime * normalSpeed * 4;
	vs_out.normalCoord2.xy = texcoords.xz * texScale * 4 + normalTime * normalSpeed * 8;

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(displaced_vertex, 1.0);
}
