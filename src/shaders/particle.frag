R"zzz(
#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec4 color;

// Ouput data
out vec4 fragment_color;

// uniform sampler2D myTextureSampler;

void main(){
	// Output color = color of the texture at the specified UV
	// color = texture( myTextureSampler, UV ) * particlecolor;
    fragment_color = color;

}
)zzz"
