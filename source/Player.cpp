#include "Player.h"

Player::Player(Camera& camera, World& world, TextureManager* textureManager)
    : camera(camera), world(world), textureManager(textureManager), selectedBlockType(0),
    position(camera.getPosition()), velocity(0.0f), size(0.6f, 1.8f, 0.6f),
    isOnGround(false), gravity(-22.0), jumpStrength(7.2f), movementSpeed(5.0f) {}

void Player::handleMouseInput(GLint button, GLint action, bool isGUIEnabled)
{
	if (isGUIEnabled) return;

	if (action == GLFW_PRESS)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			removeBlock();
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			placeBlock();
		}
	}
}

void Player::processInput(GLFWwindow* window, bool& isGUIEnabled, bool& escapeKeyPressedLastFrame, GLfloat& lastX, GLfloat& lastY)
{
    if (freezePlayer) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ||
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {

            freezePlayer = false;
        }
    }
    if (!freezePlayer)
    {
        bool isEscapePressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (isEscapePressed && !escapeKeyPressedLastFrame) {
            isGUIEnabled = !isGUIEnabled;

            glfwSetInputMode(window, GLFW_CURSOR, isGUIEnabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

            if (!isGUIEnabled) {
                GLdouble xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                lastX = static_cast<GLfloat>(xpos);
                lastY = static_cast<GLfloat>(ypos);
            }
        }
        escapeKeyPressedLastFrame = isEscapePressed;

        if (!isGUIEnabled) {
            glm::vec3 movement(0.0f);

            glm::vec3 lookDirection = camera.getLookDirection();
            lookDirection.y = 0.0f;
            if (glm::length(lookDirection) > 0.0f)
                lookDirection = glm::normalize(lookDirection);

            glm::vec3 rightDirection = camera.getRight();
            rightDirection.y = 0.0f;
            if (glm::length(rightDirection) > 0.0f)
                rightDirection = glm::normalize(rightDirection);

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                movement += lookDirection;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                movement -= lookDirection;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                movement -= rightDirection;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                movement += rightDirection;

            if (glm::length(movement) > 0.0f)
                movement = glm::normalize(movement);

            bool isShiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
            bool isCtrlPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

            GLfloat currentSpeed = flying ? movementSpeed * 5.0f : movementSpeed;
            if (isShiftPressed && !flying) currentSpeed += 1.8f;

            if (isCtrlPressed && !flying && !isInUnderwater())
            {
                currentSpeed /= 2.0f;
                if (size.y != 1.5f)
                {
                    position.y -= (1.8f - 1.5f) / 2.0f;
                    size = { 0.6f, 1.5f, 0.6f };
                }
            }
            else if (!isCtrlPressed && !flying && !isInUnderwater())
            {
                if (size.y != 1.8f)
                {
                    glm::vec3 testPosition = position;
                    testPosition.y += (1.8f - 1.5f) / 2.0f;

                    glm::vec3 oldSize = size;
                    size = { 0.6f, 1.8f, 0.6f };
                    bool canStandUp = !checkCollision(testPosition);

                    if (canStandUp)
                    {
                        position.y += (1.8f - 1.5f) / 2.0f;
                        size = { 0.6f, 1.8f, 0.6f };
                    }
                    else
                    {
                        size = oldSize;
                    }
                }
            }

            if (isInWater())
            {
                currentSpeed *= 0.5f;
            }

            // Set horizontal velocity
            velocity.x = movement.x * currentSpeed;
            velocity.z = movement.z * currentSpeed;

            if (flying) {
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                    velocity.y = currentSpeed; // Fly up
                }
                else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                    velocity.y = -currentSpeed; // Fly down
                }
                else {
                    velocity.y = 0.0f;
                }
            }
            else {
                // Regular jumping
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isOnGround) {
                    velocity.y = jumpStrength;
                    isOnGround = false;
                }
            }
        }
    }
}

bool Player::rayCast(glm::vec3& hitPos, glm::vec3& hitNormal, GLint& blockType) const
{
    glm::vec3 rayOrigin = camera.getPosition();
    glm::vec3 rayDir = getLookDirection();

    GLfloat distance = 0.0f;
    while (distance < rayCastReach) {
        glm::vec3 currentPos = rayOrigin + rayDir * distance;

        int16_t blockX = static_cast<int16_t>(std::floor(currentPos.x));
        int16_t blockY = static_cast<int16_t>(std::floor(currentPos.y));
        int16_t blockZ = static_cast<int16_t>(std::floor(currentPos.z));

        int16_t chunkX = (blockX >= 0) ? blockX / CHUNK_SIZE : (blockX + 1) / CHUNK_SIZE - 1;
        int16_t chunkZ = (blockZ >= 0) ? blockZ / CHUNK_SIZE : (blockZ + 1) / CHUNK_SIZE - 1;

        int16_t localX = (blockX % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
        int16_t localY = blockY;
        int16_t localZ = (blockZ % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;

        if (Chunk* chunk = world.getChunk(chunkX, chunkZ)) {
            blockType = chunk->getBlockType(localX, localY, localZ);
            if (blockType != -1 && blockType != WATER) {
                hitPos = glm::vec3(blockX, blockY, blockZ);

                // Determine which face was hit
                glm::vec3 blockCenter(blockX + 0.5f, blockY + 0.5f, blockZ + 0.5f);
                glm::vec3 diff = currentPos - blockCenter;

                hitNormal = glm::vec3(0);
                if (std::abs(diff.x) > std::abs(diff.y) && std::abs(diff.x) > std::abs(diff.z)) {
                    hitNormal.x = diff.x > 0 ? 1.0f : -1.0f;
                }
                else if (std::abs(diff.y) > std::abs(diff.x) && std::abs(diff.y) > std::abs(diff.z)) {
                    hitNormal.y = diff.y > 0 ? 1.0f : -1.0f;
                }
                else {
                    hitNormal.z = diff.z > 0 ? 1.0f : -1.0f;
                }
                return true;
            }
        }
        distance += rayCastStep;
    }
    return false;
}

void Player::removeBlock() 
{
    glm::vec3 hitPos, hitNormal;
    GLint blockType;

    if (rayCast(hitPos, hitNormal, blockType)) 
    {
        bool addedToInventory = addItemToInventory(blockType);

        world.setBlock(hitPos.x, hitPos.y, hitPos.z, -1);

        glm::vec3 blockAbove = hitPos + glm::vec3(0, 1, 0);
        int16_t chunkX = (blockAbove.x >= 0) ? static_cast<int16_t>(blockAbove.x / CHUNK_SIZE) : static_cast<int16_t>((blockAbove.x + 1) / CHUNK_SIZE) - 1;
        int16_t chunkZ = (blockAbove.z >= 0) ? static_cast<int16_t>(blockAbove.z / CHUNK_SIZE) : static_cast<int16_t>((blockAbove.z + 1) / CHUNK_SIZE) - 1;

        int16_t localX = static_cast<int16_t>(blockAbove.x) - (chunkX * CHUNK_SIZE);
        int16_t localY = static_cast<int16_t>(std::floor(blockAbove.y));
        int16_t localZ = static_cast<int16_t>(blockAbove.z) - (chunkZ * CHUNK_SIZE);
        localX = (localX + CHUNK_SIZE) % CHUNK_SIZE;
        localZ = (localZ + CHUNK_SIZE) % CHUNK_SIZE;

        if (Chunk* chunk = world.getChunk(chunkX, chunkZ)) 
        {
            GLint blockAboveType = chunk->getBlockType(localX, localY, localZ);
            if (blockAboveType == FLOWER1 || blockAboveType == FLOWER2 || blockAboveType == FLOWER3 || blockAboveType == FLOWER4 || blockAboveType == FLOWER5
                || blockAboveType == GRASS1 || blockAboveType == GRASS2 || blockAboveType == GRASS3 || blockAboveType == DEADBUSH || blockAboveType == TORCH)
            {
                chunk->setBlockType(localX, localY, localZ, -1);
            }
        }

        if (!addedToInventory) return;
    }
}

void Player::placeBlock() {
    glm::vec3 hitPos, hitNormal;
    GLint blockType;
    if (rayCast(hitPos, hitNormal, blockType)) {

        const InventorySlot& selectedSlot = inventory[selectedInventorySlot];

        if (selectedSlot.blockType == -1 || selectedSlot.stackSize <= 0) {
            return;
        }

        GLint blockToPlace = selectedSlot.blockType;
        glm::vec3 placePos = hitPos + hitNormal;
        glm::vec3 blockToReplace = hitPos;

        int16_t chunkX = (blockToReplace.x >= 0) ? static_cast<int16_t>(blockToReplace.x / CHUNK_SIZE) : static_cast<int16_t>((blockToReplace.x + 1) / CHUNK_SIZE) - 1;
        int16_t chunkZ = (blockToReplace.z >= 0) ? static_cast<int16_t>(blockToReplace.z / CHUNK_SIZE) : static_cast<int16_t>((blockToReplace.z + 1) / CHUNK_SIZE) - 1;

        int16_t localX = static_cast<int16_t>(blockToReplace.x) - (chunkX * CHUNK_SIZE);
        int16_t localY = static_cast<int16_t>(std::floor(blockToReplace.y));
        int16_t localZ = static_cast<int16_t>(blockToReplace.z) - (chunkZ * CHUNK_SIZE);

        localX = (localX + CHUNK_SIZE) % CHUNK_SIZE;
        localZ = (localZ + CHUNK_SIZE) % CHUNK_SIZE;

        if (Chunk* chunk = world.getChunk(chunkX, chunkZ)) {
            GLint blockType = chunk->getBlockType(localX, localY, localZ);

            if (blockToPlace == FLOWER1 || blockToPlace == FLOWER2 || blockToPlace == FLOWER3 ||
                blockToPlace == FLOWER4 || blockToPlace == FLOWER5 || blockToPlace == GRASS1 ||
                blockToPlace == GRASS2 || blockToPlace == GRASS3) {
                if (blockType != DIRT && blockType != GRASS_BLOCK) {
                    return; // Only place on dirt or grass block
                }
            }

            if (blockToPlace == DEADBUSH)
            {
                if (blockType != SAND)
                {
                    return;
                }
            }

            if ((blockType == WATER || blockType == GRASS1 ||
                blockType == GRASS2 || blockType == GRASS3 || blockType == DEADBUSH) 
                && !isBlockInsidePlayer(placePos)) {
                chunk->setBlockType(localX, localY, localZ, blockToPlace);

                inventory[selectedInventorySlot].stackSize--;
                if (inventory[selectedInventorySlot].stackSize <= 0) {
                    inventory[selectedInventorySlot].blockType = -1;
                }
                return;
            }
        }

        if (!isBlockInsidePlayer(placePos)) {
            world.setBlock(placePos.x, placePos.y, placePos.z, blockToPlace);

            inventory[selectedInventorySlot].stackSize--;
            if (inventory[selectedInventorySlot].stackSize <= 0) {
                inventory[selectedInventorySlot].blockType = -1;
            }
        }
    }
}

void Player::setSelectedBlockType(uint8_t type)
{
    selectedBlockType = type;
}

uint8_t Player::getSelectedBlockType() const
{
	return selectedBlockType;
}

void Player::setSelectedInventorySlot(int8_t slot)
{
    if (slot > 0) {
        // Scroll down
        selectedInventorySlot = (selectedInventorySlot + 1) % 9;
    }
    else if (slot < 0) {
        // Scroll up
        selectedInventorySlot = (selectedInventorySlot == 0) ? 8 : selectedInventorySlot - 1;
    }
}

bool Player::addItemToInventory(GLint blockType) {
    int8_t slotIndex = findAvailableSlot(blockType);

    if (slotIndex == -1) {
        return false;
    }

    inventory[slotIndex].blockType = blockType;
    inventory[slotIndex].stackSize++;
    return true;
}

GLint Player::findAvailableSlot(GLint blockType) {
    for (int8_t i = 0; i < inventory.size(); i++) {
        if (inventory[i].blockType == blockType && inventory[i].stackSize < 64) {
            return i;
        }
    }

    for (int8_t i = 0; i < inventory.size(); i++) {
        if (inventory[i].blockType == -1) {
            return i;
        }
    }
    return -1;
}

InventorySlot Player::getInventorySlot(uint8_t index) const {
    if (index < inventory.size()) {
        return inventory[index];
    }
    return InventorySlot();
}

GLuint Player::getTextureForBlock(GLint blockType) const
{
    if (textureManager)
    {
        return textureManager->getInventoryTextureID(blockType);
    }
    return 0;
}

glm::vec3 Player::getLookDirection() const
{
	return glm::normalize(camera.getLookDirection());
}

void Player::update(GLfloat deltaTime) {
    if (freezePlayer) {
        velocity = glm::vec3(0.0f);
        return;
    }
    
    if (!flying) {
        // Apply gravity when not flying
        if (!isOnGround) {
            velocity.y += gravity * deltaTime;
        }
    }

    glm::vec3 newPosition = position + velocity * deltaTime;

    if (!flying) {
        resolveCollisions(newPosition, deltaTime);
    }
    else {
        position = newPosition;
    }

    camera.setPosition(position + glm::vec3(0.0f, size.y / 2.0f, 0.0f));

    if (isOnGround && velocity.y < 0) {
        velocity.y = 0.0f;
    }
}

void Player::resolveCollisions(glm::vec3& newPosition, GLfloat deltaTime) {
    isOnGround = false;

    glm::vec3 futurePosition;

    futurePosition = position;
    futurePosition.y = newPosition.y;

    if (checkCollision(futurePosition)) {
        if (velocity.y < 0.0f) {
            isOnGround = true;
        }
        newPosition.y = position.y;
        velocity.y = 0.0f;
    }

    futurePosition = position;
    futurePosition.x = newPosition.x;
    futurePosition.y = newPosition.y;

    if (checkCollision(futurePosition)) {
        newPosition.x = position.x;
        velocity.x = 0.0f;
    }

    futurePosition = position;
    futurePosition.z = newPosition.z;
    futurePosition.y = newPosition.y;
    futurePosition.x = newPosition.x;

    if (checkCollision(futurePosition)) {
        newPosition.z = position.z;
        velocity.z = 0.0f;
    }

    position = newPosition;
}

bool Player::isBlockInsidePlayer(const glm::vec3& blockPos) const {
    // Get the player's AABB
    glm::vec3 playerMin = position - size * 0.5f;
    glm::vec3 playerMax = position + size * 0.5f;

    glm::vec3 blockMin = glm::vec3(std::floor(blockPos.x), std::floor(blockPos.y), std::floor(blockPos.z));
    glm::vec3 blockMax = blockMin + glm::vec3(1.0f, 1.0f, 1.0f);

    bool xOverlap = (playerMin.x < blockMax.x) && (playerMax.x > blockMin.x);
    bool yOverlap = (playerMin.y < blockMax.y) && (playerMax.y > blockMin.y);
    bool zOverlap = (playerMin.z < blockMax.z) && (playerMax.z > blockMin.z);

    return xOverlap && yOverlap && zOverlap;
}

bool Player::checkCollision(const glm::vec3& position) {
    glm::vec3 min = position - size * 0.5f;
    glm::vec3 max = position + size * 0.5f;

    for (GLint x = static_cast<GLint>(std::floor(min.x)); x <= static_cast<GLint>(std::floor(max.x)); ++x) {
        for (GLint y = static_cast<GLint>(std::floor(min.y)); y <= static_cast<GLint>(std::floor(max.y)); ++y) {
            for (GLint z = static_cast<GLint>(std::floor(min.z)); z <= static_cast<GLint>(std::floor(max.z)); ++z) {
                int16_t chunkX = (x >= 0) ? x / CHUNK_SIZE : (x + 1) / CHUNK_SIZE - 1;
                int16_t chunkZ = (z >= 0) ? z / CHUNK_SIZE : (z + 1) / CHUNK_SIZE - 1;

                int16_t localX = (x % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
                int16_t localY = y;
                int16_t localZ = (z % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;

                if (Chunk* chunk = world.getChunk(chunkX, chunkZ)) {
                    GLint blockType = chunk->getBlockType(localX, localY, localZ);
                    // Ignore non-solid blocks
                    if (blockType == -1 || blockType == WATER || blockType == FLOWER1 || blockType == FLOWER2 || blockType == FLOWER3 || blockType == FLOWER4 || blockType == FLOWER5
                        || blockType == GRASS1 || blockType == GRASS2 || blockType == GRASS3 || blockType == DEADBUSH || blockType == TORCH) {
                        continue; // No collision
                    }
                    // Collision detected
                    return true;
                }
            }
        }
    }

    // No collision
    return false;
}

void Player::setFlying(bool enableFlying) {
    flying = enableFlying;
    if (flying) {
        velocity.y = 0.0f;
    }
}

bool Player::isFlying() const {
    return flying;
}

bool Player::isInWater() const {
    glm::vec3 playerMin = position - size * 0.5f;
    glm::vec3 playerMax = position + size * 0.5f;

    for (int16_t x = static_cast<int16_t>(std::floor(playerMin.x)); x <= static_cast<int16_t>(std::floor(playerMax.x)); ++x) {
        for (int16_t y = static_cast<int16_t>(std::floor(playerMin.y)); y <= static_cast<int16_t>(std::floor(playerMax.y)); ++y) {
            for (int16_t z = static_cast<int16_t>(std::floor(playerMin.z)); z <= static_cast<int16_t>(std::floor(playerMax.z)); ++z) {
                int16_t chunkX = (x >= 0) ? x / CHUNK_SIZE : (x + 1) / CHUNK_SIZE - 1;
                int16_t chunkZ = (z >= 0) ? z / CHUNK_SIZE : (z + 1) / CHUNK_SIZE - 1;

                int16_t localX = (x % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
                int8_t localY = y;
                int16_t localZ = (z % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;

                if (Chunk* chunk = world.getChunk(chunkX, chunkZ)) {
                    int16_t blockType = chunk->getBlockType(localX, localY, localZ);
                    if (blockType == WATER) {
                        return true; // Player is in water
                    }
                }
            }
        }
    }
    return false; // Player is not in water
}

bool Player::isInUnderwater() const {
    glm::vec3 playerMin = position - size * 0.5f;
    glm::vec3 playerMax = position + size * 0.5f;

    GLfloat headHeight = playerMax.y - (size.y * 0.5f);

    for (int16_t x = static_cast<int16_t>(std::floor(playerMin.x)); x <= static_cast<int16_t>(std::floor(playerMax.x)); ++x) {
        for (int16_t y = static_cast<int16_t>(std::floor(headHeight)); y <= static_cast<int16_t>(std::floor(playerMax.y)); ++y) {
            for (int16_t z = static_cast<int16_t>(std::floor(playerMin.z)); z <= static_cast<int16_t>(std::floor(playerMax.z)); ++z) {
                int16_t chunkX = (x >= 0) ? x / CHUNK_SIZE : (x + 1) / CHUNK_SIZE - 1;
                int16_t chunkZ = (z >= 0) ? z / CHUNK_SIZE : (z + 1) / CHUNK_SIZE - 1;

                int16_t localX = (x % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
                int8_t localY = y;
                int16_t localZ = (z % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;

                if (Chunk* chunk = world.getChunk(chunkX, chunkZ)) {
                    int16_t blockType = chunk->getBlockType(localX, localY, localZ);
                    if (blockType != WATER) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

/* //Old implementation of placeBlock though Imgui
void Player::placeBlock() {
    glm::vec3 hitPos, hitNormal;
    GLint blockType;
    if (rayCast(hitPos, hitNormal, blockType)) {
        glm::vec3 placePos = hitPos + hitNormal;
        glm::vec3 blockToReplace = hitPos;

        int16_t chunkX = (blockToReplace.x >= 0) ? static_cast<int16_t>(blockToReplace.x / CHUNK_SIZE) : static_cast<int16_t>((blockToReplace.x + 1) / CHUNK_SIZE) - 1;
        int16_t chunkZ = (blockToReplace.z >= 0) ? static_cast<int16_t>(blockToReplace.z / CHUNK_SIZE) : static_cast<int16_t>((blockToReplace.z + 1) / CHUNK_SIZE) - 1;

        int16_t localX = static_cast<int16_t>(blockToReplace.x) - (chunkX * CHUNK_SIZE);
        int16_t localY = static_cast<int16_t>(std::floor(blockToReplace.y));
        int16_t localZ = static_cast<int16_t>(blockToReplace.z) - (chunkZ * CHUNK_SIZE);

        localX = (localX + CHUNK_SIZE) % CHUNK_SIZE;
        localZ = (localZ + CHUNK_SIZE) % CHUNK_SIZE;

        if (Chunk* chunk = world.getChunk(chunkX, chunkZ)) {
            GLint blockType = chunk->getBlockType(localX, localY, localZ);

            if (selectedBlockType == FLOWER1 || selectedBlockType == FLOWER2 || selectedBlockType == FLOWER3 || selectedBlockType == FLOWER4 || selectedBlockType == FLOWER5
                || selectedBlockType == GRASS1 || selectedBlockType == GRASS2 || selectedBlockType == GRASS3) {
                if (blockType != DIRT && blockType != GRASS_BLOCK) {
                    return; // Flower only can be placed on dirt or grass block
                }
            }

            if ((blockType == WATER || blockType == GRASS1 || blockType == GRASS2 || blockType == GRASS3 || blockType == DEADBUSH) && !isBlockInsidePlayer(placePos)) {
                chunk->setBlockType(localX, localY, localZ, selectedBlockType);
                return;
            }
        }

        if (!isBlockInsidePlayer(placePos)) {
            world.setBlock(placePos.x, placePos.y, placePos.z, selectedBlockType);
        }
    }
}
*/