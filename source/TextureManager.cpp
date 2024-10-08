#include "TextureManager.h"

TextureManager::TextureManager()
{
    loadTextures();
}

TextureManager::~TextureManager()
{
    glDeleteTextures(1, &textureID);
}

GLuint TextureManager::getTextureID() const
{
    return textureID;
}

void TextureManager::loadTextures()
{
    GLint width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

    unsigned char* data = stbi_load(texturePaths[0].c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        std::cerr << "Failed to load texture: " << texturePaths[0] << std::endl;
        return;
    }
    stbi_image_free(data);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, width, height, texturePaths.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    for (size_t i = 0; i < texturePaths.size(); ++i) {
        data = stbi_load(texturePaths[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (data) {
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cerr << "Failed to load texture: " << texturePaths[i] << std::endl;
        }
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}