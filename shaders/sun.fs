#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sunTexture;

void main() {
    vec4 texColor = texture(sunTexture, TexCoords);
    if(texColor.a < 0.001)
        discard;
    FragColor = texColor;
}