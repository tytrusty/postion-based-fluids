R"zzz(
#version 430 core

in vec2 UV;

out vec4 fragment_color;

uniform sampler2D tex_depth;

void main(){
    float depth = texture(tex_depth, UV).r;
    if (depth > 0.99f)
        discard;
    fragment_color = vec4(depth,depth,depth, 1.0f);
}
)zzz"
