R"zzz(
#version 430 core

in vec2 UV;

out vec4 fragment_color;

uniform sampler2D tex_normal;
uniform sampler2D tex_depth;
uniform vec4 light_position;
uniform mat4 view;
uniform mat4 projection;

vec3 diffuse = vec3(64/255.0,124/255.0, 1.0);
vec3 ambient = vec3(0.1, 0.1, 0.3);

float far = 10.0f;
float near = 0.1f;
vec3 uv_to_view(vec2 UV, float depth)
{
    float x = 2.0 * UV.x - 1.0;
    float y = 2.0 * UV.y - 1.0;
    float z = depth;
    vec4 view_pos = inverse(projection) * vec4(x,y,z,1.0);
    return view_pos.xyz / view_pos.w;
}

float linearize(float depth)
{
    return (2.0*near) / (far+near - depth*(far-near));
}

void main(){
    vec3 normal = texture(tex_normal, UV).rgb;
    fragment_color=vec4(normal,1.0);
    if (normal == vec3(1,1,1))
        discard;

    mat4 inv_view = inverse(view);

    vec3 light = vec3(view*light_position);
    vec3 pos = uv_to_view(UV, texture(tex_depth,UV).r);
    vec3 world_pos = vec3(inv_view*vec4(pos,1.0f));
    vec3 light_dir = vec3(normalize(vec3(view*light_position)));//-world_pos));
    vec3 world_normal = vec3(vec4(normal,0.0f));

    float dot_nl = clamp(dot(light_dir, world_normal), 0.0, 1.0);
    vec3 color = clamp(dot_nl*diffuse + ambient, 0.0, 1.0);
    fragment_color = vec4(color, 1.0f);
}
)zzz"
