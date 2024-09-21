#version 330 core
out vec4 FragColor;

in vec2 texCoord;
in float texLayer;
in float fogFactor;  // Fog factor from the vertex shader
in vec3 FragPos;
in vec3 Normal;
in float LightLevel;
in float ao;

uniform sampler2DArray texArray;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform vec3 moonDirection;
uniform vec3 moonColor;

uniform vec4 fogColor;  // Fog color, including alpha

void main()
{
    // Base texture color
    vec3 textureColor = texture(texArray, vec3(texCoord, texLayer)).rgb;

    // Ambient lighting
    float minAmbient = 0.23;
    float ambientStrength = mix(minAmbient, 0.23, LightLevel);
    vec3 ambient = (ambientStrength * lightColor) * ao;

    // Initialize lighting result
    vec3 lighting = ambient;

    // Sun Lighting
    float fadeFactorSun = clamp(-lightDirection.y, 0.0, 1.0);
    vec3 norm = normalize(Normal);
    float diffSun = max(dot(norm, -lightDirection), 0.0) * fadeFactorSun * LightLevel;
    vec3 diffuseSun = (diffSun * lightColor) * ao;

    // Moon Lighting
    float fadeFactorMoon = clamp(-moonDirection.y, 0.0, 1.0);
    float diffMoon = max(dot(norm, -moonDirection), 0.0) * fadeFactorMoon * LightLevel;
    vec3 diffuseMoon = (diffMoon * moonColor) * ao;

    // Specular lighting (Sun and Moon)
    float specularStrength = 0.5 * LightLevel;
    vec3 viewDir = normalize(viewPos - FragPos);

    // Sun Specular
    vec3 reflectDirSun = reflect(lightDirection, norm);
    float specSun = pow(max(dot(viewDir, reflectDirSun), 0.0), 32) * fadeFactorSun * LightLevel;
    vec3 specularSun = specularStrength * specSun * lightColor;

    // Moon Specular
    vec3 reflectDirMoon = reflect(moonDirection, norm);
    float specMoon = pow(max(dot(viewDir, reflectDirMoon), 0.0), 32) * fadeFactorMoon * LightLevel;
    vec3 specularMoon = specularStrength * specMoon * moonColor;

    // Combine lighting components (Sun + Moon)
    lighting += (diffuseSun + specularSun) + (diffuseMoon + specularMoon);

    // Compute the final lighting result
    vec3 result = lighting * textureColor;

    // --- FOG APPLICATION ---
    // Blend the final result color with the fog color based on the fogFactor
    vec3 finalColor = mix(fogColor.rgb, result, fogFactor);

    // Output the final color with full alpha (1.0)
    FragColor = vec4(finalColor, 1.0);
}
