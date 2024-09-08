#include "Camera.h"

Camera::Camera() : view(calcView()) {}

const glm::mat4& Camera::updateView() {
    return view = calcView();
}

const glm::mat4& Camera::lookAt(glm::vec3 eye, glm::vec3 center) {
    position = eye;
    updateCameraDirection(center - eye);
    return updateView();
}

const glm::mat4& Camera::setPosition(glm::vec3 eye) {
    position = eye;
    return updateView();
}

void Camera::updateCameraDirection(const glm::vec3& newForward) {
    glm::vec3 forward = glm::normalize(newForward);
    lookDirection = forward;

    directionVectors[Direction::FORWARD] = forward;
    directionVectors[Direction::BACKWARD] = -forward;
    directionVectors[Direction::RIGHT] = glm::normalize(glm::cross(forward, cameraUp));
    directionVectors[Direction::LEFT] = -directionVectors[Direction::RIGHT];
}

void Camera::updateCameraOrientation(GLfloat newYaw, GLfloat newPitch) {
    yaw = newYaw;
    pitch = glm::clamp(newPitch, -89.0f, 89.0f);

    glm::vec3 front;
    front.x = glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
    front.y = glm::sin(glm::radians(pitch));
    front.z = glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));

    updateCameraDirection(glm::normalize(front));

    updateView();
}

glm::mat4 Camera::calcView() const {
    return glm::lookAt(position, position + lookDirection, cameraUp);
}

void Camera::setMovementState(Direction dir, bool isMoving) {
    movementStates[dir] = isMoving;
}

glm::vec3 Camera::getMoveDirection() const {
    glm::vec3 moveDirection(0.0f);

    for (const auto& [direction, isMoving] : movementStates) {
        if (isMoving) {
            moveDirection += directionVectors.at(direction);
        }
    }

    return moveDirection;
}

void Camera::update(GLfloat deltaTime) {
    glm::vec3 moveDirection = getMoveDirection();
    position += moveDirection * deltaTime * movementSpeed;
    updateView();
}