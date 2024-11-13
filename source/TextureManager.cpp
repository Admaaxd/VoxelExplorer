#include "TextureManager.h"

TextureManager::TextureManager()
{
    loadInventoryTextures();
    loadTextures();
}

TextureManager::~TextureManager()
{
    glDeleteTextures(1, &textureID);
    glDeleteTextures(static_cast<GLsizei>(inventoryTextureIDs.size()), inventoryTextureIDs.data());
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

GLuint TextureManager::getInventoryTextureID(GLint blockType) const
{
    switch (blockType)
    {
    case DIRT:
        return inventoryTextureIDs[0];
    case STONE:
        return inventoryTextureIDs[1];
    case GRASS_BLOCK:
        return inventoryTextureIDs[2];
    case SAND:
        return inventoryTextureIDs[4];
    case WATER:
        return inventoryTextureIDs[5];
    case OAK_LOG:
        return inventoryTextureIDs[7];
    case OAK_LEAF:
        return inventoryTextureIDs[8];
    case GRAVEL:
        return inventoryTextureIDs[9];
    case COBBLESTONE:
        return inventoryTextureIDs[10];
    case GLASS:
        return inventoryTextureIDs[11];
    case FLOWER1:
        return inventoryTextureIDs[12];
    case GRASS1:
        return inventoryTextureIDs[13];
    case GRASS2:
        return inventoryTextureIDs[14];
    case GRASS3:
        return inventoryTextureIDs[15];
    case FLOWER2:
        return inventoryTextureIDs[16];
    case FLOWER3:
        return inventoryTextureIDs[17];
    case FLOWER4:
        return inventoryTextureIDs[18];
    case FLOWER5:
        return inventoryTextureIDs[19];
    case OAK_LEAF_ORANGE:
        return inventoryTextureIDs[20];
    case OAK_LEAF_YELLOW:
        return inventoryTextureIDs[21];
    case DEADBUSH:
        return inventoryTextureIDs[22];
    case OAK_LEAF_PURPLE:
        return inventoryTextureIDs[23];
    case SNOW:
        return inventoryTextureIDs[24];
    case TORCH:
        return inventoryTextureIDs[25];
    default:
        return 0;
    }
}

void TextureManager::loadInventoryTextures()
{
    GLint width, height, channels;
    stbi_set_flip_vertically_on_load(false);

    inventoryTextureIDs.resize(texturePaths.size());
    glGenTextures(static_cast<GLsizei>(inventoryTextureIDs.size()), inventoryTextureIDs.data());

    for (size_t i = 0; i < texturePaths.size(); ++i)
    {
        glBindTexture(GL_TEXTURE_2D, inventoryTextureIDs[i]);

        unsigned char* data = stbi_load(texturePaths[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        else
        {
            std::cerr << "Failed to load texture: " << texturePaths[i] << std::endl;
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}