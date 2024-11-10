#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include "stb_image.h"
#include "BlockTypes.h"
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

    GLuint getInventoryTextureID(GLint blockType) const;

private:
    GLuint textureID;
    void loadTextures();
    std::vector<std::string> textures;

    std::vector<GLuint> inventoryTextureIDs;
    void loadInventoryTextures();

    const std::vector<std::string> texturePaths = 
    {
        "textures/dirt.jpg",                // DIRT -> 0
        "textures/stone.jpg",               // STONE -> 1
        "textures/grass_top.jpg",           // GRASS BLOCK TOP -> 2
        "textures/grass_side.jpg",          // GRASS BLOCK SIDE -> 3
        "textures/sand.jpg",                // SAND -> 4
        "textures/water.jpg",               // WATER -> 5
        "textures/oak_log_top_bottom.jpg",  // OAK LOG TOP AND BOTTOM -> 6
        "textures/oak_log_side.jpg",        // OAK LOG SIDE -> 7
        "textures/oak_leaf.png",            // OAK LEAF -> 8
        "textures/gravel.jpg",              // GRAVEL -> 9
        "textures/cobblestone.jpg",         // COBBLESTONE -> 10
        "textures/glass.png",               // GLASS -> 11
        "textures/flower1.png",             // FLOWER 1 -> 12
        "textures/grass1.png",              // GRASS 1 -> 13
        "textures/grass2.png",              // GRASS 2 -> 14
        "textures/grass3.png",              // GRASS 3 -> 15
        "textures/flower2.png",             // FLOWER 2 -> 16
        "textures/flower3.png",             // FLOWER 3 -> 17
        "textures/flower4.png",             // FLOWER 4 -> 18
        "textures/flower5.png",             // FLOWER 5 -> 19
        "textures/oak_leaf_orange.png",     // OAK LEAF ORANGE -> 20
        "textures/oak_leaf_yellow.png",     // OAK LEAF YELLOW -> 21
        "textures/deadbush.png",            // DEADBUSH -> 22
        "textures/oak_leaf_purple.png",     // OAK LEAF PURPLE -> 23
        "textures/snow.jpg",                // SNOW BLOCK -> 24

    };
};

#endif // TEXTURE_MANAGER_H