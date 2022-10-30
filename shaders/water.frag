#version 410

uniform samplerCube cubemap;
uniform sampler2D normal_texture;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_model_to_world;
uniform vec3 light_position;
uniform vec3 camera_position;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	float x_deriv;
	float z_deriv;
	vec2 normalCoord0;
	vec2 normalCoord1;
	vec2 normalCoord2;
} fs_in;

out vec4 frag_color;

void main() {
	// Surface calculations
	vec3 V = normalize(camera_position - fs_in.vertex);
	//vec3 L = normalize(light_position - V);
	vec4 color_deep = vec4(0.0, 0.0, 0.1, 1.0);
	vec4 color_shallow = vec4(0.0, 0.5, 0.5, 1.0);
	vec3 N = normalize(vec3(-fs_in.x_deriv, 1.0, -fs_in.z_deriv));

	// Normal calculations
	vec3 T = vec3(1.0, fs_in.x_deriv, 0.0);
	vec3 B = vec3(0.0, fs_in.z_deriv, 1.0);
	mat4 TBN_surface = mat4(vec4(T, 1.0), vec4(B, 1.0), vec4(N, 1.0), vec4(0.0));

	vec4 n_bump = texture(normal_texture, fs_in.normalCoord0) * 2 - 1;
	n_bump += texture(normal_texture, fs_in.normalCoord1) * 2 - 1;
	n_bump += texture(normal_texture, fs_in.normalCoord2) * 2 - 1;
	n_bump = normalize(n_bump);

	mat4 TBN_water = mat4(vec4(T, 1.0), vec4(B, 1.0), n_bump, vec4(0.0));

	N = vec3(vertex_model_to_world * TBN_surface * TBN_water * n_bump);

	float facing = 1.0f - max(dot(V, N), 0);

	vec3 R = reflect(-V, N);
	vec4 reflection = texture(cubemap, R);

	// Fresnel
	float R0 = 0.02037;
	float fresnel = R0 + (1 - R0) * pow((1 - dot(V, N)), 5);

	float eta = 1.0 / 1.33;
	vec3 R2 = refract(-V, N, eta);
	vec4 refraction = texture(cubemap, R2);

	frag_color = mix(color_deep, color_shallow, facing) + (reflection * fresnel) + (refraction * (1 - fresnel));
}
