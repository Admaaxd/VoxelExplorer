#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <array>

struct MovementDirection {
	bool isMoving = false;
	glm::vec3 direction = glm::vec3(0);
};

class Camera {
	glm::mat4 view = calcView();
	glm::vec3 position = { 0, 90, 0 };
	glm::vec3 cameraUp = { 0, 1, 0 };

	MovementDirection forward = { false, {1, 0, 0} };
	MovementDirection backward = { false, {-1, 0, 0} };
	MovementDirection left = { false, {0, 0, 1} };
	MovementDirection right = { false, {0, 0, -1} };
	MovementDirection up = { false, {0, 1, 0} };
	MovementDirection down = { false, {0, -1, 0} };
	glm::vec3 lookDirection = forward.direction;

	GLfloat yaw = 0;
	GLfloat pitch = 0.5;

	glm::mat4 calcView() const;
	const glm::mat4& updateView();

public:
	GLfloat movementSpeed = 15.0f;

	const glm::mat4& lookAt(glm::vec3 eye, glm::vec3 center);
	void update(GLfloat deltaTime);

	void updateCameraDirection(glm::vec3 newForward);
	void updateCameraOrientation(GLfloat yaw, GLfloat pitch);

	[[nodiscard]] const glm::mat4& getViewMatrix() const { return view; }

	[[nodiscard]] GLfloat getYaw() const { return yaw; };
	[[nodiscard]] GLfloat getPitch() const { return pitch; };

	[[nodiscard]] glm::vec3 getLookDirection() const;
	[[nodiscard]] glm::vec3 getPosition() const;
	const glm::mat4& setPosition(glm::vec3 eye);

	void setIsMovingForward(bool isMoving) { forward.isMoving = isMoving; };
	void setIsMovingBackward(bool isMoving) { backward.isMoving = isMoving; };
	void setIsMovingLeft(bool isMoving) { left.isMoving = isMoving; };
	void setIsMovingRight(bool isMoving) { right.isMoving = isMoving; };
	void setIsMovingUp(bool isMoving) { up.isMoving = isMoving; };
	void setIsMovingDown(bool isMoving) { down.isMoving = isMoving; };
	glm::vec3 getMoveDirection();
};
#endif