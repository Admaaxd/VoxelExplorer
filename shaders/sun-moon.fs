#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sunTexture;

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

    FragColor = texColor;
}
