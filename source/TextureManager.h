#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include "stb_image.h"
#include <vector>
#include <string>
#include <iostream>
#include <glad/glad.h>

class TextureManager
{
public:
    TextureManager();
    ~TextureManager();
    GLuint getTextureID() const;

private:
    GLuint textureID;
    std::vector<std::string> textures;
    void loadTextures();

    const std::vector<std::string> texturePaths = 
    {
        "textures/dirt.jpg", // DIRT -> 0
        "textures/stone.jpg", // STONE -> 1
        "textures/grass_top.jpg", // GRASS TOP -> 2
        "textures/grass_side.jpg", // GRASS SIDE -> 3
        "textures/sand.jpg", // SAND -> 4
        "textures/water.jpg", // WATER -> 5
    };
};

#endif // TEXTURE_MANAGER_H