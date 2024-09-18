#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
layout (location = 2) in float aTexLayer;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in float aLightLevel;

out vec2 texCoord;
out float texLayer;
out vec3 FragPos;
out vec3 Normal;
out float LightLevel;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPos;

    texCoord = aTex;
    texLayer = aTexLayer;
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    LightLevel = aLightLevel;
}
