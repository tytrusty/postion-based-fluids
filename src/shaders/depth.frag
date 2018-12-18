R"zzz(
#version 430 core

in vec2 UV;

out vec4 fragment_color;
uniform sampler2D tex_depth;

float far = 10.0f;
float near = 0.1f;

float linearize(float depth)
{
    return (2.0*near) / (far+near - depth*(far-near));
}

void main(){
    float depth = texture(tex_depth, UV).r;
    if (depth > 0.999)
        discard;
    depth = linearize(depth);
    fragment_color = vec4(depth,depth,depth, 1.0f);
}
)zzz"
