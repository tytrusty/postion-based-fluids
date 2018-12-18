R"zzz(
#version 430 core

in vec2 UV;

out vec4 out_normal;

uniform sampler2D tex_depth;
uniform vec2 pixel_size;

void main(){
    float depth = texture(tex_depth, UV).r;
    if (depth < 1e-4)
        discard;

    vec2 left  = vec2(UV.x-pixel_size.x, UV.y);
    vec2 right = vec2(UV.x+pixel_size.x, UV.y);
    vec2 up    = vec2(UV.x, UV.y+pixel_size.y);
    vec2 down  = vec2(UV.x, UV.y-pixel_size.y);
    
    float dz_dx = (texture(tex_depth, right).r-texture(tex_depth, left).r) / 2.0; 
    float dz_dy = (texture(tex_depth, up).r-texture(tex_depth, down).r) / 2.0; 
    vec3 dir = normalize(vec3(-dz_dx, -dz_dy, 1.0));
    out_normal = vec4(dir, 1.0);
}
)zzz"
