#include "Crosshair.h"

Crosshair::Crosshair() : crosshairVAO(0), crosshairVBO(0) {}

Crosshair::~Crosshair() {
    Delete();
}

void Crosshair::initialize() {
    GLfloat crosshairVertices[] = {
        -0.02f,  0.0f,   // Left horizontal line
         0.02f,  0.0f,   // Right horizontal line
         0.0f,   0.02f,  // Top vertical line
         0.0f,  -0.02f   // Bottom vertical line
    };

    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);

    glBindVertexArray(crosshairVAO);

    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Crosshair::render(shader& crosshairShader, const glm::vec3& color, GLfloat size) const {
    crosshairShader.use();

    glUniform3f(glGetUniformLocation(crosshairShader.ID, "crosshairColor"), color.x, color.y, color.z);

    GLfloat crosshairVertices[] = {
        -0.01f * size,  0.0f,
         0.01f * size,  0.0f,
         0.0f,  0.02f * size,
         0.0f, -0.02f * size
    };

    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(crosshairVertices), crosshairVertices);

    glBindVertexArray(crosshairVAO);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);
}

void Crosshair::Delete() {
    glDeleteVertexArrays(1, &crosshairVAO);
    glDeleteBuffers(1, &crosshairVBO);
}