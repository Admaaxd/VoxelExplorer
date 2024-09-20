#include "SkyboxRenderer.h"

SkyboxRenderer::SkyboxRenderer(const std::vector<std::string>& dayFaces, const std::vector<std::string>& nightFaces, const std::string& sunTexturePath, const std::string& moonTexturePath)
    : skyboxShader("shaders/skybox.vs", "shaders/skybox.fs"), sunMoonShader("shaders/sun-moon.vs", "shaders/sun-moon.fs"),
    orbitCenter(glm::vec3(0.0f, 0.0f, 0.0f)), orbitRadius(300.0f), orbitSpeed(0.01f), currentAngle(0.0f)
{
    dayCubemapTexture = loadCubemap(dayFaces);
    nightCubemapTexture = loadCubemap(nightFaces);
    setupSkybox();

    sunTexture = loadTexture(sunTexturePath);
    setupSun();
    sunPosition = glm::vec3(100.0f, 100.0f, -100.0f);

    moonTexture = loadTexture(moonTexturePath);
    setupMoon();
}

SkyboxRenderer::~SkyboxRenderer() {
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteTextures(1, &dayCubemapTexture);
    glDeleteTextures(1, &nightCubemapTexture);

    glDeleteVertexArrays(1, &sunVAO);
    glDeleteBuffers(1, &sunVBO);
    glDeleteTextures(1, &sunTexture);
}

void SkyboxRenderer::updateSunAndMoonPosition(float deltaTime) {
    currentAngle += orbitSpeed * deltaTime;

    sunPosition.x = orbitCenter.x;
    sunPosition.y = orbitCenter.y + orbitRadius * sin(currentAngle);
    sunPosition.z = orbitCenter.z + orbitRadius * cos(currentAngle);

    moonPosition = -sunPosition;

    if (sunPosition.y < 0.0f) isNight = true;
    else isNight = false;
}

void SkyboxRenderer::setOrbitSpeed(float speed)
{
    orbitSpeed = speed;
}

float SkyboxRenderer::getOrbitSpeed() const {
    return orbitSpeed;
}

void SkyboxRenderer::setupSkybox() {
    GLfloat skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void SkyboxRenderer::setupSun() {
    GLfloat sunVertices[] = {
        // positions        // texture coords
       -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
       -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        1.0f, -1.0f, 0.0f,  1.0f, 0.0f,

       -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &sunVAO);
    glGenBuffers(1, &sunVBO);

    glBindVertexArray(sunVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sunVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sunVertices), sunVertices, GL_STATIC_DRAW);

    // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    // texture coord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
        (void*)(3 * sizeof(GLfloat)));

    glBindVertexArray(0);
}

void SkyboxRenderer::setupMoon() {
    GLfloat moonVertices[] = {
        // positions        // texture coords
       -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
       -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        1.0f, -1.0f, 0.0f,  1.0f, 0.0f,

       -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &moonVAO);
    glGenBuffers(1, &moonVBO);

    glBindVertexArray(moonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, moonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(moonVertices), moonVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    glBindVertexArray(0);
}

GLuint SkyboxRenderer::loadCubemap(const std::vector<std::string>& faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    GLint width, height, nrChannels;
    for (GLuint i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cerr << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

GLuint SkyboxRenderer::loadTexture(const std::string& path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    GLint width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load texture at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void SkyboxRenderer::renderSkybox(const glm::mat4& view, const glm::mat4& projection) {
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    skyboxShader.use();

    GLfloat time = glfwGetTime();
    GLfloat rotationSpeed = 0.2f;
    GLfloat rotationAngle = glm::radians(rotationSpeed * time);

    glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);

    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis);

    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
    skyboxView = skyboxView * rotation;
    skyboxShader.setMat4("view", skyboxView);
    skyboxShader.setMat4("projection", projection);

    glBindVertexArray(skyboxVAO);
    if (isNight) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, nightCubemapTexture);
    }
    else {
        glBindTexture(GL_TEXTURE_CUBE_MAP, dayCubemapTexture);
    }
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
}

void SkyboxRenderer::renderSun(const glm::mat4& view, const glm::mat4& projection) {
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    sunMoonShader.use();
    sunMoonShader.setMat4("projection", projection);
    sunMoonShader.setMat4("view", view);

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, sunPosition);

    glm::mat3 viewRotation = glm::mat3(view);
    glm::mat3 invViewRotation = glm::transpose(viewRotation);

    model *= glm::mat4(invViewRotation);

    GLfloat sunSize = 20.0f;
    model = glm::scale(model, glm::vec3(sunSize));

    sunMoonShader.setMat4("model", model);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sunTexture);
    sunMoonShader.setInt("sunTexture", 0);

    glBindVertexArray(sunVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

glm::vec3 SkyboxRenderer::getSunPosition() const {
    return sunPosition;
}

void SkyboxRenderer::renderMoon(const glm::mat4& view, const glm::mat4& projection) {
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    sunMoonShader.use();
    sunMoonShader.setMat4("projection", projection);
    sunMoonShader.setMat4("view", view);

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, moonPosition);

    glm::mat3 viewRotation = glm::mat3(view);
    glm::mat3 invViewRotation = glm::transpose(viewRotation);

    model *= glm::mat4(invViewRotation);

    GLfloat moonSize = 10.0f;
    model = glm::scale(model, glm::vec3(moonSize));

    sunMoonShader.setMat4("model", model);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, moonTexture);
    sunMoonShader.setInt("sunTexture", 0);

    glBindVertexArray(moonVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

glm::vec3 SkyboxRenderer::getMoonPosition() const {
    return moonPosition;
}
