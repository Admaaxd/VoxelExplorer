#ifndef SKYBOX_RENDERER_H
#define SKYBOX_RENDERER_H

#include <stb/stb_image.h>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include "shader.h"

class SkyboxRenderer {
public:
    SkyboxRenderer(const std::vector<std::string>& faces);
    ~SkyboxRenderer();

    void render(const glm::mat4& view, const glm::mat4& projection);

private:
    GLuint skyboxVAO, skyboxVBO;
    GLuint cubemapTexture;
    shader skyboxShader;

    GLuint loadCubemap(const std::vector<std::string>& faces);
    void setupSkybox();

};

#endif // SKYBOX_RENDERER_H