R"zzz(
#version 430 core

in vec2 UV;

out vec4 fragment_color;

uniform sampler2D tex_depth;
uniform vec4 light_position;
uniform mat4 view;

vec3 diffuse = vec3(64/255.0,124/255.0, 1.0);
vec3 ambient = vec3(0.1, 0.1, 0.3);

void main(){
    vec3 normal = texture(tex_depth, UV).rgb;
    if (length(normal) < 1e-4)
        discard;

	float dot_nl = dot(normalize(vec3(view*light_position)), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	vec3 color = clamp(dot_nl*diffuse + ambient, 0.0, 1.0);
    fragment_color = vec4(color, 1.0f);
}
)zzz"
