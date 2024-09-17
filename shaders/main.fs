#version 330 core
out vec4 FragColor;

in vec2 texCoord;
in float texLayer;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2DArray texArray;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{
    // Ambient lighting
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(lightDirection, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 textureColor = texture(texArray, vec3(texCoord, texLayer)).rgb;

    vec3 result = (ambient + diffuse + specular) * textureColor;
    FragColor = vec4(result, 1.0);
}
