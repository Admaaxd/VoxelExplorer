#ifndef CROSSHAIR_H
#define CROSSHAIR_H

#include <glad/glad.h>
#include <glm.hpp>
#include "shader.h"

class Crosshair {
public:
    Crosshair();
    ~Crosshair();

    void initialize();
    void render(shader& crosshairShader, const glm::vec3& color, GLfloat size) const;
    void Delete();

private:
    GLuint crosshairVAO;
    GLuint crosshairVBO;
};

#endif // CROSSHAIR_H