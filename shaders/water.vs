#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in float aTexLayer;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in float aLightLevel;
layout(location = 5) in float aAO;

out vec2 TexCoords;
out float TexLayer;
out vec3 Normal;
out float LightLevel;
out float AO;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoord;
    TexLayer = aTexLayer;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    LightLevel = aLightLevel;
    AO = aAO;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
