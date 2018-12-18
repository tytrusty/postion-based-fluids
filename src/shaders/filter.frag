R"zzz(
#version 430 core

in vec2 UV;

uniform vec2 pixel_size;
uniform sampler2D tex_depth;
uniform float radius;
uniform int filter_radius;
uniform float filter_sigma;

out vec4 out_depth;

float gaussian2D(vec2 r, float sigma2)
{
    return exp(-dot(r,r)/sigma2);
}

float gaussian1D(float r, float sigma2)
{
    return exp(-r*r/sigma2);
}

void main() {
    float depth_center = texture(tex_depth, UV).r;
    out_depth.a = 1.0;
    if (depth_center > 0.99 || depth_center < 0.001)
    {
        out_depth.r = depth_center;
        return;
        // discard;
    }

    float sigma = filter_sigma*radius;
    float sigma2 = sigma*sigma*2.0;

    float x1 = UV.x, x2 = UV.x, y1 = UV.y, y2 = UV.y;

    float filtered_depth = depth_center;
    float weight_sum = 1.0;

    for (int i = 1; i <= filter_radius; ++i)
    {
        x1 += pixel_size.x;
        x2 -= pixel_size.x;
        
        for (int j = 1; j <= filter_radius; ++j)
        {
            y1 += pixel_size.y;
            y2 -= pixel_size.y;

            //// TODO optimize?
            vec2 right_up   = vec2(x1, y1);
            vec2 right_down = vec2(x1, y2);
            vec2 left_up    = vec2(x2, y1);
            vec2 left_down  = vec2(x2, y2);

            // depths 
            float d1 = texture(tex_depth, right_up).r;
            float d2 = texture(tex_depth, right_down).r;
            float d3 = texture(tex_depth, left_up).r;
            float d4 = texture(tex_depth, left_down).r;

            // weights
            float w1 = gaussian1D(d1, sigma2);
            float w2 = gaussian1D(d2, sigma2);
            float w3 = gaussian1D(d3, sigma2);
            float w4 = gaussian1D(d4, sigma2);

            // radial weight (decreasing weight with distance from center)
            float w_r = gaussian2D(vec2(i*pixel_size.x, j*pixel_size.y), sigma2);

            filtered_depth += w_r * (w1*d1 + w2*d2 + w3*d3 + w4*d4);
            weight_sum += w_r * (w1 + w2 + w3 + w4);
        }
    }
    filtered_depth /= weight_sum;
    out_depth.r = filtered_depth;
}
)zzz"
