#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in float TexLayer;
in vec3 Normal;
in float LightLevel;
in float AO;

uniform sampler2DArray textureArray;
uniform vec3 lightDirection;
uniform vec3 viewPos;

void main()
{
    vec4 texColor = texture(textureArray, vec3(TexCoords, TexLayer));

    vec3 norm = normalize(Normal);

    float diff = max(dot(norm, -lightDirection), 0.0);

    vec3 color = texColor.rgb * diff * LightLevel * AO;

    float alpha = 0.5;

    FragColor = vec4(color, alpha);

    if (FragColor.a < 0.1)
        discard;
}
