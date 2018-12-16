R"zzz(
#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 color;

// Ouput data
out vec4 fragment_color;

uniform vec4 light_position;
uniform float radius;
uniform mat4 view;

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

	vec3 normal = normalize(vec3(ndc, normalized_depth));
    vec3 pos = normal*radius + gl_FragCoord.xyz;
    vec3 light_dir = normalize(vec3(light_position) - pos);
	vec3 final_color = color;

    vec3 view_dir = -normalize(vec3(view*vec4(pos,1.0f)));

    // light direction, in view space
    light_dir = normalize(vec3(view*(light_position)));//-vec4(pos,1.0f))));

	// ambient + diffuse
	float lightingIntensity = 0.3 + 0.7 * clamp(dot(light_dir, normal), 0.0, 1.0);
	final_color *= lightingIntensity;

	// Per fragment specular lighting
	// vec4 spec = specular * pow(max(0.0, dot(reflect(-light_direction, vertex_normal), camera_direction)), shininess);
    // TODO camera_position - view_pos  ... in world coordinates
    lightingIntensity = clamp(dot(reflect(-light_dir, normal), view_dir), 0.0, 1.0);
	lightingIntensity = pow(lightingIntensity, 60.0);
	final_color += vec3(0.4, 0.4, 0.4) * lightingIntensity;

	fragment_color = vec4(final_color, 1.0);

}
)zzz"
