#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
layout (location = 2) in float aTexLayer;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in float aLightLevel;
layout (location = 5) in float aAO;

out vec2 texCoord;
out float texLayer;
out vec3 FragPos;
out vec3 Normal;
out float fogFactor;
out float LightLevel;
out float ao;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPosition;  // Camera position to calculate distance for fog

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPos;

    texCoord = aTex;
    texLayer = aTexLayer;
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    LightLevel = aLightLevel;
    ao = aAO;

    // Compute distance from the camera to the fragment position for fog
    float distance = length(worldPos.xyz - cameraPosition);

    // Fog parameters
    float fogStart = 45.0;  // Fog starts at this distance
    float fogEnd = 180.0;   // Fog fully takes over at this distance

    // Calculate the fog factor (linear interpolation between fogStart and fogEnd)
    fogFactor = clamp((fogEnd - distance) / (fogEnd - fogStart), 0.0, 1.0);
}