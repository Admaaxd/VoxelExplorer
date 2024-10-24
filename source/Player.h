#pragma once

#include "World.h"
#include "Camera.h"

class Player {
public:
	Player(Camera& camera, World& world);

	void handleMouseInput(GLint button, GLint action, bool isGUIEnabled);
	void processInput(GLFWwindow* window, bool& isGUIEnabled, bool& escapeKeyPressedLastFrame, GLfloat& lastX, GLfloat& lastY);
	bool rayCast(glm::vec3& hitPos, glm::vec3& hitNormal, GLint& blockType) const;
	void removeBlock();
	void placeBlock();
	void setSelectedBlockType(uint8_t type);
	uint8_t getSelectedBlockType() const;

	void update(GLfloat deltaTime);

	bool checkCollision(const glm::vec3& position);
	void resolveCollisions(glm::vec3& velocity, GLfloat deltaTime);
	bool isBlockInsidePlayer(const glm::vec3& blockPos) const;
	void setFlying(bool enableFlying);
	bool isFlying() const;
	bool isInWater() const;

private:
	glm::vec3 getLookDirection() const;

	Camera& camera;
	World& world;

	uint8_t selectedBlockType;

	const GLfloat rayCastStep = 0.001f; // The step size for ray traversal
	const GLfloat rayCastReach = 6.0f; // How far the ray can reach

	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 size;

	GLfloat movementSpeed;
	GLfloat jumpStrength;

	bool isOnGround;
	GLfloat gravity;
	bool flying = true;
};