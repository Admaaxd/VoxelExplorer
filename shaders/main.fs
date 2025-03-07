#version 330 core
out vec4 FragColor;

in vec2 texCoord;
in float texLayer;
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

uniform vec4 fogColor;
uniform bool isUnderwater;

void main()
{
    // Sample the texture color
    vec4 texColor = texture(texArray, vec3(texCoord, texLayer));

    if (texColor.a < 0.95)
        discard;

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
    vec3 result = lighting * texColor.rgb;

    // Compute distance from the camera to the fragment position for fog
    float distance = length(FragPos - viewPos);

    // Initialize final color
    vec3 finalColor = result;

    // Apply underwater effect
    if (isUnderwater)
    {
        float depthFactor = clamp((FragPos.y - viewPos.y) * -0.05, 0.0, 1.0);
        vec3 underwaterColor = vec3(0.0, 0.3, 0.6);
        vec3 underwaterEffect = mix(result, underwaterColor, depthFactor);

        float fogDensity = 0.05;
        float fogFactorUnderwater = exp(-distance * fogDensity);
        underwaterEffect = mix(underwaterColor, underwaterEffect, fogFactorUnderwater);

        finalColor = underwaterEffect;
    }
    else
    {
        float fogDensity = 0.0084;

        float fogFactor = exp(-pow(distance * fogDensity, 2.0));

        fogFactor = clamp(fogFactor, 0.0, 1.0);

        finalColor = mix(fogColor.rgb, result, fogFactor);
    }

    // Output the final color
    FragColor = vec4(finalColor, texColor.a);
}
