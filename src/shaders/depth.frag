R"zzz(
#version 430 core

in vec2 UV;

out vec4 fragment_color;

uniform sampler2D tex_depth;

void main(){
    //float depth = texture(tex_depth, UV).r;
    //if (depth < 1e-4)
    //    discard;
    //fragment_color = vec4(depth,depth,depth, 1.0f);
    vec3 normal = texture(tex_depth, UV).rgb;
    fragment_color = vec4(normal, 1.0f);
}
)zzz"
