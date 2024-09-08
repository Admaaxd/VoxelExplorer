#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <array>
#include <unordered_map>

enum class Direction {
    FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN
};

class Camera {
    glm::mat4 view;
    glm::vec3 position = { 0, 90, 0 };
    glm::vec3 cameraUp = { 0, 1, 0 };
    glm::vec3 lookDirection = { 1, 0, 0 }; // Default forward

    std::unordered_map<Direction, glm::vec3> directionVectors = {
        {Direction::FORWARD, {1, 0, 0}},
        {Direction::BACKWARD, {-1, 0, 0}},
        {Direction::LEFT, {0, 0, 1}},
        {Direction::RIGHT, {0, 0, -1}},
        {Direction::UP, {0, 1, 0}},
        {Direction::DOWN, {0, -1, 0}},
    };

    std::unordered_map<Direction, bool> movementStates = {
        {Direction::FORWARD, false},
        {Direction::BACKWARD, false},
        {Direction::LEFT, false},
        {Direction::RIGHT, false},
        {Direction::UP, false},
        {Direction::DOWN, false}
    };

    GLfloat yaw = 0.0f;
    GLfloat pitch = 0.5f;
    GLfloat movementSpeed = 25.0f;

    glm::mat4 calcView() const;

public:
    Camera();

    const glm::mat4& updateView();
    const glm::mat4& lookAt(glm::vec3 eye, glm::vec3 center);
    const glm::mat4& setPosition(glm::vec3 eye);

    void updateCameraDirection(const glm::vec3& newForward);
    void updateCameraOrientation(GLfloat newYaw, GLfloat newPitch);
    void update(GLfloat deltaTime);

    [[nodiscard]] const glm::mat4& getViewMatrix() const { return view; }
    [[nodiscard]] GLfloat getYaw() const { return yaw; }
    [[nodiscard]] GLfloat getPitch() const { return pitch; }
    [[nodiscard]] glm::vec3 getLookDirection() const { return lookDirection; }
    [[nodiscard]] glm::vec3 getPosition() const { return position; }

    void setMovementState(Direction dir, bool isMoving);
    glm::vec3 getMoveDirection() const;
};

#endif // CAMERA_H