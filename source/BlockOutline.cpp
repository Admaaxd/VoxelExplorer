#include "BlockOutline.h"

BlockOutline::BlockOutline()
    : outlinedBlockShader("shaders/blockOutlineShader.vs", "shaders/blockOutlineShader.fs")
{
    // Vertices for a cube outline
    std::vector<glm::vec3> vertices = {
        // Front face
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f},

        // Back face
        {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f},

        // Connecting lines
        {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
        {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}
    };

    glGenVertexArrays(1, &blockOutlineVAO);
    glBindVertexArray(blockOutlineVAO);

    glGenBuffers(1, &blockOutlineVBO);
    glBindBuffer(GL_ARRAY_BUFFER, blockOutlineVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

BlockOutline::~BlockOutline() {
    glDeleteVertexArrays(1, &blockOutlineVAO);
    glDeleteBuffers(1, &blockOutlineVBO);
}

void BlockOutline::render(const glm::mat4& transform) {
    glLineWidth(3.0f);
    outlinedBlockShader.use();
    outlinedBlockShader.setMat4("MVP", transform);

    glBindVertexArray(blockOutlineVAO);
    glDrawArrays(GL_LINES, 0, 24);
    glBindVertexArray(0);
    glLineWidth(1.0f);
}