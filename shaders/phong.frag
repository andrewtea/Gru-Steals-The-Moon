uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform float shininess_value;
uniform vec3 light_position;
uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform sampler2D normal_texture;
//uniform bool use_normal_mapping;
uniform mat4 normal_model_to_world;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 texcoords;
	vec3 tangent;
	vec3 binormal;
} fs_in;

out vec4 frag_color;

void main() {
	vec3 N = normalize(fs_in.normal);
	vec3 V = normalize(fs_in.vertex);
	vec3 T = normalize(fs_in.tangent);
	vec3 B = normalize(fs_in.binormal);
	vec3 L = normalize(light_position - V);

	vec4 mapped_n = texture(normal_texture, fs_in.texcoords) * 2 - 1;
	mat4 TBN = mat4(vec4(T, 1.0), vec4(B, 1.0), vec4(N, 1.0), vec4(0.0));
	N = vec3(normal_model_to_world * TBN * mapped_n);

	vec3 R = normalize(reflect(-L, N));

	vec4 vector_diffuse = vec4(diffuse_colour, 1.0) * texture(diffuse_texture, fs_in.texcoords) * max(dot(N, L), 0.0);
	vec4 vector_specular = vec4(specular_colour, 1.0) * texture(specular_texture, fs_in.texcoords) * pow(max(dot(R,V),0.0), shininess_value);
	frag_color = vec4(ambient_colour, 0.1) + vector_diffuse + vector_specular;
}
