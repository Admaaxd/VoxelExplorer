#pragma once

#include "Block.h"
#include "TextureManager.h"
#include "Frustum.h"
#include "Structure.h"
#include "shader.h"
#include "Camera.h"
#include "Biomes.h"
#include <numeric>

class World;

constexpr uint8_t CHUNK_SIZE = 16;
constexpr uint8_t CHUNK_HEIGHT = 128;
constexpr uint8_t WATERLEVEL = 62;

struct BiomeData {
	GLfloat minThreshold, maxThreshold;
	BiomeTypes type;
	GLfloat baseFrequency;
	GLfloat caveThreshold;
	GLfloat edge0, edge1, edge2, edge3;
};

extern std::vector<BiomeData> biomes;

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

	Biomes determineBiomeType(GLint x, GLint z);
	const BiomeData* selectBiome(GLfloat noiseValue);
	const Biomes* getBiomeInstance(BiomeTypes type) const;
	GLfloat smoothstep(GLfloat edge0, GLfloat edge1, GLfloat x);

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

	FastNoiseLite biomeNoise, caveNoise;
	Biomes forestBiome, desertBiome, plainsBiome, mountainBiome;

	std::vector<BiomeData> biomes = {
		{0.0f, 0.33f, BiomeTypes::Desert, 0.0011f, 0.5f, 0.2f, 0.4f, 0.0f, 0.0f},
		{0.33f, 0.5f, BiomeTypes::Plains, 0.0003f, 0.98f, 0.3f, 0.4f, 0.6f, 0.7f},
		{0.5f, 0.8f, BiomeTypes::Forest, 0.002f, 0.85f, 0.6f, 0.8f, 0.0f, 0.0f},
		{0.8f, 1.0f, BiomeTypes::Mountain, 0.005f, 0.9f, 0.6f, 0.7f, 0.6f, 0.8f}
	};

	std::unordered_map<BiomeTypes, Biomes*> biomeInstances = {
		{BiomeTypes::Desert, &desertBiome},
		{BiomeTypes::Plains, &plainsBiome},
		{BiomeTypes::Forest, &forestBiome},
		{BiomeTypes::Mountain, &mountainBiome}
	};
};