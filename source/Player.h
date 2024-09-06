#pragma once

#include "World.h"
#include "Camera.h"

class Player {
public:
	Player(Camera& camera, World& world);

	void handleMouseInput(GLint button, GLint action, bool isGUIEnabled);
	bool rayCast(glm::vec3& hitPos, glm::vec3& hitNormal, GLint& blockType) const;
	void removeBlock();
	void placeBlock();
	void setSelectedBlockType(uint8_t type);
	uint8_t getSelectedBlockType() const;

private:
	glm::vec3 getLookDirection() const;

	Camera& camera;
	World& world;

	uint8_t selectedBlockType;

	const GLfloat rayCastStep = 0.001f; // The step size for ray traversal
	const GLfloat rayCastReach = 6.0f; // How far the ray can reach
};