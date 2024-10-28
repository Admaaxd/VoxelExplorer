#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sunTexture;
uniform bool isUnderwater;

// Blur intensity
const float blurRadius = 0.004; 
const int samples = 10;

void main() {
    vec4 texColor = vec4(0.0);
    vec2 texOffset = vec2(blurRadius);

    // Apply Gaussian blur
    for(int x = -samples; x <= samples; ++x) {
        for(int y = -samples; y <= samples; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texOffset;
            texColor += texture(sunTexture, TexCoords + offset);
        }
    }

    texColor /= float((samples * 2 + 1) * (samples * 2 + 1)); // Normalize the color

    if (texColor.a < 0.001)
        discard;

    if (isUnderwater) {
        vec3 underwaterTint = vec3(0.0, 0.3, 0.6);
        float underwaterIntensity = 0.85;
        texColor.rgb = mix(texColor.rgb, underwaterTint, underwaterIntensity);
    }

    FragColor = texColor;
}
