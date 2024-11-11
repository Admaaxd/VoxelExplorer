#pragma once

#include "Chunk.h"
#include "ThreadPool.h"
#include <unordered_map>
#include <queue>
#include <map>
#include <unordered_set>

struct BlockChange {
	int16_t localX, localY, localZ;
	uint8_t blockType;
};

class World
{
public:
	World(const Frustum& frustum);
	~World();
	bool isInitialChunksLoaded();
	void Draw(const Frustum& frustum);
	void DrawWater(const Frustum& frustum, shader& waterShader, glm::mat4 view, glm::mat4 projection, glm::vec3 lightDirection, Camera& camera);
	void updatePlayerPosition(const glm::vec3& position, const Frustum& frustum);
	void processChunkLoadQueue(uint8_t maxChunksToLoad, uint16_t delay);
	Chunk* getChunk(int16_t x, int16_t z);
	GLfloat getTerrainHeightAt(GLfloat x, GLfloat z);

	void setBlock(int16_t x, int16_t y, int16_t z, int8_t type);

	void queueBlockChange(int16_t chunkX, int16_t chunkZ, int16_t localX, int16_t localY, int16_t localZ, uint8_t blockType);

	void updateAllChunkMeshes();

	struct ChunkCoord {
		int16_t x, z;

		bool operator==(const ChunkCoord& other) const {
			return x == other.x && z == other.z;
		}

		bool operator<(const ChunkCoord& other) const {
			if (x != other.x)
				return x < other.x;
			else
				return z < other.z;
		}
	};

	struct ChunkCoordHash {
		std::size_t operator()(const ChunkCoord& coord) const {
			return std::hash<int16_t>()(coord.x) ^ std::hash<int16_t>()(coord.z);
		}
	};

	const std::unordered_map<ChunkCoord, Chunk*, ChunkCoordHash>& getChunks() const { return chunks; }

	bool isAOEnabled = true;
	bool isFrustumCullingEnabled = true;
	bool isStructureGenerationEnabled = true;

	bool getAOState() const { return isAOEnabled; }
	void setAOState(bool enabled);

	bool getFrustumCullingState() const { return isFrustumCullingEnabled; }
	void setFrustumCullingState(bool enabled);
	
	bool getStructureGenerationState() const { return isStructureGenerationEnabled; }
	void setStructureGenerationState(bool enabled);

private:
	struct ChunkCoordComparator {
		ChunkCoordComparator(const World& world) : world(world) {}
		bool operator()(const ChunkCoord& a, const ChunkCoord& b) const;
		const World& world;
	};

	void queueChunkLoad(int16_t x, int16_t z);
	void loadChunk(int16_t x, int16_t z);
	void unloadChunk(int16_t x, int16_t z);
	bool isChunkLoaded(int16_t x, int16_t z);
	bool isWithinRenderDistance(int16_t x, int16_t z) const;
	void updateNeighboringChunksOnBlockChange(int16_t chunkX, int16_t chunkZ, int16_t localX, int16_t localY, int16_t localZ);
	bool isChunkInFrustum(int16_t chunkX, int16_t chunkZ, const Frustum& frustum) const;

	void propagateSunlight(int16_t chunkX, int16_t chunkZ, int16_t localX, int16_t localY, int16_t localZ);

	void addChunk(Chunk* chunk);
	void notifyNeighbors(Chunk* chunk);

	std::vector<BlockChange> getQueuedBlockChanges(int16_t chunkX, int16_t chunkZ);

	std::unordered_map<ChunkCoord, Chunk*, ChunkCoordHash> chunks;
	std::priority_queue<ChunkCoord, std::vector<ChunkCoord>, ChunkCoordComparator> chunkLoadQueue;
	int16_t playerChunkX, playerChunkZ;
	TextureManager textureManager;

	std::unordered_map<ChunkCoord, std::future<Chunk*>, ChunkCoordHash> pendingChunks;
	std::mutex chunksMutex;
	ThreadPool threadPool;

	std::map<ChunkCoord, std::vector<BlockChange>> queuedBlockChanges;
	std::mutex queuedBlockChangesMutex;

	const uint8_t renderDistance = 12;

};