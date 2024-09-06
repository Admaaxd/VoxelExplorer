#pragma once

#include "shader.h"
#include "glm/glm.hpp"
#include <vector>

class BlockOutline {
public:
    BlockOutline();
    ~BlockOutline();

    void render(const glm::mat4& transform);

private:
    shader outlinedBlockShader;
    GLuint blockOutlineVAO, blockOutlineVBO;
};