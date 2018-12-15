R"zzz(
#version 330 core

in vec3 vertex_position;
in vec3 particle_position;
in vec4 particle_color;

out vec2 UV;
out vec3 color;

uniform mat4 projection;
uniform mat4 view;
uniform float radius;

void main()
{
	vec3 particleCenter = particle_position.xyz;
	
    vec3 left = vec3(view[0].x, view[1].x, view[2].x); 
    vec3 up   = vec3(view[0].y, view[1].y, view[2].y); 

	vec3 vertex_world = particleCenter 
        + left * vertex_position.x * radius
		+ up   * vertex_position.y * radius;

	gl_Position = projection * view * vec4(vertex_world, 1.0f);

	UV = vertex_position.xy + vec2(0.5, 0.5);
	color = vec3(particle_color);
}
)zzz"
