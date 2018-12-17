R"zzz(
#version 430 core

in vec3 vertex_position;
out vec2 UV;

void main()
{
	gl_Position =  vec4(vertex_position, 1.0);
	UV = (vertex_position.xy+vec2(1,1))/2.0;
}
)zzz"
