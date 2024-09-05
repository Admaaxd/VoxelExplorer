#version 330 core
out vec4 FragColor;

uniform vec3 crosshairColor;

void main()
{
    FragColor = vec4(crosshairColor, 1.0);
}
