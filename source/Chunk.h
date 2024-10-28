#pragma once

#include "Block.h"
#include "FastNoiseLite.h"
#include "TextureManager.h"
#include "Frustum.h"
#include "Structure.h"
#include "shader.h"
#include "Camera.h"

class World;

constexpr uint8_t CHUNK_SIZE = 16;
constexpr uint8_t CHUNK_HEIGHT = 128;
constexpr uint8_t WATERLEVEL = 62;

enum BlockType {
	DIRT,
	STONE,
	GRASS_BLOCK,
	SAND,
	WATER,
	OAK_LOG,
	OAK_LEAF,
	GRAVEL,
	COBBLESTONE,
	GLASS,
	FLOWER1,
	GRASS1,
	GRASS2,
	GRASS3,
	FLOWER2
};

enum FaceType {
	TOP = 4,
	BOTTOM = 5,
	SIDE
};

class Chunk
{
public:
	Chunk(GLint x, GLint z, TextureManager& textureManager, World* world);
	~Chunk();

	void Draw();
	void DrawWater(shader& waterShader, glm::mat4 view, glm::mat4 projection, glm::vec3 lightDirection, Camera& camera);
	void setupChunk();
	void updateOpenGLBuffers();
	void updateOpenGLWaterBuffers();

	void generateMesh(const std::vector<GLint>& blockTypes);
	GLint getBlockType(GLint x, GLint y, GLint z) const;
	const std::vector<GLint>& getBlockTypes() const;
	void setBlockType(GLint x, GLint y, GLint z, int8_t type);
	GLint getTerrainHeightAt(GLint x, GLint z);

	bool isInFrustum(const Frustum& frustum) const;
	glm::vec3 getMinBounds() const { return minBounds; }
	glm::vec3 getMaxBounds() const { return maxBounds; }

	GLint getChunkX() const { return chunkX; }
	GLint getChunkZ() const { return chunkZ; }

	bool isLoaded() const { return isInitialized; }

	void recalculateSunlightColumn(GLint x, GLint z);

	GLfloat chunkX, chunkZ;
	std::vector<GLint> blockTypes;
	std::vector<uint8_t> lightLevels;

	World* world;
	bool needsMeshUpdate = false;

private:
	void generateChunk();
	void calculateBounds();
	void placeBlockIfInChunk(GLint globalX, GLint y, GLint globalZ, GLint blockType);
	inline GLint getIndex(GLint x, GLint y, GLint z) const;
	inline bool isTransparent(GLint blockType);
	GLint getTextureLayer(int8_t blockType, int8_t face);

	void addGrassPlant(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, GLint x, GLint y, GLint z, uint8_t lightLevel, GLint blockType);

	FastNoiseLite baseNoise, elevationNoise, caveNoise, ridgeNoise, detailNoise, mountainNoise, treeNoise, treeHeightNoise, grassNoise, flowerNoise;
	void initializeNoise();

	GLuint textureID;
	FastNoiseLite noiseGenerator;
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	GLuint VAO = 0, VBO = 0, EBO = 0;

	std::vector<GLfloat> waterVertices;
	std::vector<GLuint> waterIndices;
	GLuint waterVAO = 0, waterVBO = 0, waterEBO = 0;

	glm::vec3 minBounds;
	glm::vec3 maxBounds;
	bool isInitialized = false;

	std::vector<std::vector<std::vector<GLint>>> chunkData;
	TextureManager& textureManager;

};