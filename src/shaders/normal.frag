R"zzz(
#version 430 core

in vec2 UV;

out vec4 out_normal;

uniform sampler2D tex_depth;
uniform vec2 pixel_size;
uniform mat4 projection;


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
    float depth = texture(tex_depth, UV).r;
    if (depth > 0.999 || depth < 1e-4)
    {
        out_normal = vec4(1,1,1,1);
        return;
    }

    // UV coords
    vec2 left  = vec2(UV.x-pixel_size.x, UV.y);
    vec2 right = vec2(UV.x+pixel_size.x, UV.y);
    vec2 up    = vec2(UV.x, UV.y+pixel_size.y);
    vec2 down  = vec2(UV.x, UV.y-pixel_size.y);

    // adjacent depth values
    float dl = linearize(texture(tex_depth, left).r);
    float dr = linearize(texture(tex_depth, right).r);
    float du = linearize(texture(tex_depth, up).r);
    float dd = linearize(texture(tex_depth, down).r);

    // position and adjacent positions in view coordinates
    vec3 pos       = uv_to_view(UV, depth);
    vec3 pos_right = uv_to_view(right, dr);
    vec3 pos_left  = uv_to_view(left, dl);
    vec3 pos_up    = uv_to_view(up, du);
    vec3 pos_down  = uv_to_view(down, dd);

    // gradient approx
    vec3 dx_left = pos - pos_left;
    vec3 dx_right = pos_right - pos;
    vec3 dx = (abs(dx_right.z) < abs(dx_left.z)) ? dx_right : dx_left;

    vec3 dy_up   = pos_up - pos;
    vec3 dy_down = pos - pos_down;
    vec3 dy = (abs(dy_up.z) < abs(dy_down.z)) ? dy_up : dy_down;

    // cross gradients to get normal direction
    out_normal = vec4(-normalize(cross(dx,dy)),1.0);
}
)zzz"
