#include "Player.h"

Player::Player(Camera& camera, World& world) : camera(camera), world(world), selectedBlockType(0) {}

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
            if (blockType != -1) {
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
        world.setBlock(hitPos.x, hitPos.y, hitPos.z, -1);
    }
}

void Player::placeBlock()
{
    glm::vec3 hitPos, hitNormal;
    GLint blockType;
    if (rayCast(hitPos, hitNormal, blockType)) {
        glm::vec3 placePos = hitPos + hitNormal;
        world.setBlock(placePos.x, placePos.y, placePos.z, selectedBlockType);
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

glm::vec3 Player::getLookDirection() const
{
	return glm::normalize(camera.getLookDirection());
}
