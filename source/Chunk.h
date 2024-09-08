#pragma once

#include "Block.h"
#include "FastNoiseLite.h"
#include "TextureManager.h"
#include "Frustum.h"

class World;

constexpr uint8_t CHUNK_SIZE = 16;
constexpr uint8_t CHUNK_HEIGHT = 128;
constexpr uint8_t WATERLEVEL = 62;

class Chunk
{
public:
	Chunk(int16_t x, int16_t z, TextureManager& textureManager, World* world);
	~Chunk();

	void Draw();
	void setupChunk();
	void updateOpenGLBuffers();

	void generateMesh(const std::vector<GLint>& blockTypes);
	GLint getBlockType(uint8_t x, uint8_t y, uint8_t z) const;
	const std::vector<GLint>& getBlockTypes() const;
	void setBlockType(GLint x, GLint y, GLint z, int8_t type);

	bool isInFrustum(const Frustum& frustum) const;
	glm::vec3 getMinBounds() const { return minBounds; }
	glm::vec3 getMaxBounds() const { return maxBounds; }

	GLfloat chunkX, chunkZ;
	std::vector<GLint> blockTypes;

private:
	void generateChunk();
	void calculateBounds();

	GLuint textureID;
	FastNoiseLite noiseGenerator;
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	GLuint VAO, VBO, EBO;

	glm::vec3 minBounds;
	glm::vec3 maxBounds;

	std::vector<std::vector<std::vector<GLint>>> chunkData;

	TextureManager& textureManager;

	World* world;
};