#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform bool isUnderwater;

void main()
{    
    vec4 color = texture(skybox, TexCoords);

    if (isUnderwater) {
        color = mix(color, vec4(0.0, 0.3, 0.7, 1.0), 0.8);
    }

    FragColor = color;
}