#version 330 core

out vec4 FragColor;

in vec3 FragPos;
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

    vec3 viewDir = normalize(viewPos - FragPos);

    float cosTheta = max(dot(viewDir, norm), 0.0);
    float F0 = 0.8;
    float fresnelFactor = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

    vec3 reflectionColor = vec3(0.5, 0.7, 0.9);
    vec3 refractionColor = texColor.rgb;

    vec3 color = mix(refractionColor, reflectionColor, fresnelFactor);

    float diff = max(dot(norm, -lightDirection), 0.0);
    color *= diff * LightLevel * AO;

    float alpha = 0.5;

    FragColor = vec4(color, alpha);

    if (FragColor.a < 0.1)
        discard;
}
