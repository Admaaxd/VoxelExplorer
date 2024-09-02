#version 330 core
out vec4 FragColor;

in vec2 texCoord;
in float texLayer;

uniform sampler2DArray texArray;

void main()
{
	  FragColor = texture(texArray, vec3(texCoord, texLayer));
}
