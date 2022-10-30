uniform samplerCube cubemap;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 V = normalize(fs_in.vertex);
	vec3 N = normalize(fs_in.normal);
	vec3 R = reflect(-V, N);
	vec4 reflection = texture(cubemap, R);
	frag_color = reflection;
}

