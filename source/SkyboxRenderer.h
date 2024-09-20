#ifndef SKYBOX_RENDERER_H
#define SKYBOX_RENDERER_H

#include <stb/stb_image.h>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include "shader.h"

class SkyboxRenderer {
public:
    SkyboxRenderer(const std::vector<std::string>& faces, const std::string& sunTexturePath, const std::string& moonTexturePath);
    ~SkyboxRenderer();

    void renderSkybox(const glm::mat4& view, const glm::mat4& projection);
    void renderSun(const glm::mat4& view, const glm::mat4& projection);
    void renderMoon(const glm::mat4& view, const glm::mat4& projection);
    glm::vec3 getSunPosition() const;
    glm::vec3 getMoonPosition() const;
    void updateSunAndMoonPosition(float deltaTime);

    void setOrbitSpeed(float speed);
    float getOrbitSpeed() const;

private:
    GLuint skyboxVAO, skyboxVBO;
    GLuint cubemapTexture;
    shader skyboxShader;

    GLuint sunVAO, sunVBO;
    GLuint sunTexture;
    shader sunMoonShader;
    glm::vec3 sunPosition;

    GLuint moonVAO, moonVBO;
    GLuint moonTexture;
    glm::vec3 moonPosition;

    glm::vec3 orbitCenter;
    GLfloat orbitRadius;
    GLfloat orbitSpeed;
    GLfloat currentAngle;

    GLuint loadCubemap(const std::vector<std::string>& faces);
    void setupSkybox();

    GLuint loadTexture(const std::string& path);
    void setupSun();
    void setupMoon();

};

#endif // SKYBOX_RENDERER_H