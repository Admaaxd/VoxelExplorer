#pragma once

#include "Block.h"
#include "FastNoiseLite.h"
#include "TextureManager.h"
#include "Frustum.h"
#include "Structure.h"

class World;

constexpr uint8_t CHUNK_SIZE = 16;
constexpr uint8_t CHUNK_HEIGHT = 128;
constexpr uint8_t WATERLEVEL = 62;

class Chunk
{
public:
	Chunk(GLint x, GLint z, TextureManager& textureManager, World* world);
	~Chunk();

	void Draw();
	void setupChunk();
	void updateOpenGLBuffers();

	void generateMesh(const std::vector<GLint>& blockTypes);
	GLint getBlockType(GLint x, GLint y, GLint z) const;
	const std::vector<GLint>& getBlockTypes() const;
	void setBlockType(GLint x, GLint y, GLint z, int8_t type);

	bool isInFrustum(const Frustum& frustum) const;
	glm::vec3 getMinBounds() const { return minBounds; }
	glm::vec3 getMaxBounds() const { return maxBounds; }

	void recalculateSunlightColumn(GLint x, GLint z);

	GLfloat chunkX, chunkZ;
	std::vector<GLint> blockTypes;
	std::vector<uint8_t> lightLevels;

	World* world;
	bool needsMeshUpdate = false;

private:
	void generateChunk();
	void calculateBounds();
	GLint getTerrainHeightAt(GLint x, GLint z);
	void placeBlockIfInChunk(GLint globalX, GLint y, GLint globalZ, GLint blockType);
	GLint getIndex(GLint x, GLint y, GLint z);
	GLint getTextureLayer(int8_t blockType, int8_t face);

	void addGrassPlant(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, GLint x, GLint y, GLint z, uint8_t lightLevel, GLint blockType);

	FastNoiseLite baseNoise, elevationNoise, caveNoise, ridgeNoise, detailNoise, mountainNoise, treeNoise, treeHeightNoise, grassNoise, flowerNoise;
	void initializeNoise();

	GLuint textureID;
	FastNoiseLite noiseGenerator;
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	GLuint VAO, VBO, EBO;

	glm::vec3 minBounds;
	glm::vec3 maxBounds;
	bool isInitialized = false;

	std::vector<std::vector<std::vector<GLint>>> chunkData;
	TextureManager& textureManager;

};