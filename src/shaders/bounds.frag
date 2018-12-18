R"zzz(
#version 330 core
in vec4 face_normal;
in vec4 vertex_normal;
in vec4 light_direction;
in vec4 world_position;
out vec4 fragment_color;

vec3 ambient = vec3(0.2, 0.2, 0.3);
vec3 diffuse = vec3(0.6, 0.6, 0.6);
vec3 specular = vec3(0.296648, 0.296648, 0.296648);
float shininess = 11.264;

void main() {
    

    float thresh = 0.01f;

	vec4 pos = world_position;

	vec3 color = diffuse;
	float dot_nl = dot(normalize(light_direction), normalize(face_normal));
	dot_nl = 0.5f * clamp(dot_nl, 0.0, 1.0);
	color = clamp(dot_nl*color + ambient, 0.0, 1.0);
	fragment_color = vec4(color, 1.0);
}
)zzz"
