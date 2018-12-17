R"zzz(
#version 330 core
in vec4 face_normal;
in vec4 vertex_normal;
in vec4 light_direction;
in vec4 world_position;
out vec4 fragment_color;

vec3 ambient = vec3(0.2, 0.2, 0.3);
vec3 diffuse = vec3(0.7, 0.7, 0.7);
vec3 specular = vec3(0.296648, 0.296648, 0.296648);
float shininess = 11.264;

void main() {
    

    float thresh = 0.01f;

	vec4 pos = world_position;
	float check_width = 2.0;
	float i0 = floor(pos.x / check_width);
	float i1 = ceil(pos.x / check_width);
	float j0  = floor(pos.z / check_width);
	float j1  = ceil(pos.z / check_width);
    
    float dist_i = min(pos.x/check_width-i0, i1 - pos.x/check_width);
    float dist_j = min(pos.z/check_width-j0, j1 - pos.z/check_width);

    bool accept = dist_i<thresh || dist_j<thresh;
    accept = (2*dist_i*dist_j)<thresh;
    if (!accept)
        discard;

	vec3 color = /*mod(i + j, 2) */ diffuse;
	float dot_nl = dot(normalize(light_direction), normalize(face_normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	color = clamp(dot_nl*color + ambient, 0.0, 1.0);

    float a_dist = 1 - (dist_i*dist_j)/thresh; //min(dist_i,dist_j)/thresh; 
    a_dist = a_dist*a_dist;

	fragment_color = vec4(color, a_dist);
}
)zzz"
