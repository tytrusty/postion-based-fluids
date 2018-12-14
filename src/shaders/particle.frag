R"zzz(
#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
flat in vec4 color;

// Ouput data
out vec4 fragment_color;

uniform vec4 light_position;
uniform float radius;

void main(){
	// Output color = color of the texture at the specified UV
	// color = texture( myTextureSampler, UV ) * particlecolor;
    vec2 ndc = 2.0*UV - 1.0f;
    float dist = sqrt(dot(ndc,ndc));
    if (dist >= 1.0)
    {
        discard;
	}

	// Get fragment depth difference from center of circle, along z-axis
    float normalized_depth = sqrt(1-dist);
    float fragment_depth = radius * 0.5f * normalized_depth;

	float curr_depth = (gl_FragCoord.z - fragment_depth - 0.0025);

	vec3 normal = normalize(vec3(ndc, normalized_depth));
    vec3 pos = normal*radius + gl_FragCoord.xyz;
    vec3 light_dir = normalize(vec3(light_position) - pos);
	vec3 final_color = vec3(color) ; //vec3(1.0, 0.0, 0.0);

	// ambient
	float lightingIntensity = 0.3 + 0.7 * clamp(dot(light_dir, normal), 0.0, 1.0);
	final_color *= lightingIntensity;

	// Per fragment specular lighting
	lightingIntensity  = clamp(dot(light_dir, normal), 0.0, 1.0);
	lightingIntensity  = pow(lightingIntensity, 60.0);
	final_color += vec3(0.4, 0.4, 0.4) * lightingIntensity;

	fragment_color = vec4(final_color, 1.0);

}
)zzz"
