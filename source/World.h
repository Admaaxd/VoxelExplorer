#pragma once

#include "Chunk.h"
#include "ThreadPool.h"
#include <unordered_map>
#include <queue>

class World
{
public:
	World(const Frustum& frustum);
	~World();
	void Draw(const Frustum& frustum);
	void updatePlayerPosition(const glm::vec3& position);
	void processChunkLoadQueue(uint8_t maxChunksToLoad);
	Chunk* getChunk(GLint x, GLint z);

	void setBlock(GLint x, GLint y, GLint z, int8_t type);

	struct ChunkCoord {
		GLint x, z;
		bool operator==(const ChunkCoord& other) const {
			return x == other.x && z == other.z;
		}
	};

	struct ChunkCoordHash {
		std::size_t operator()(const ChunkCoord& coord) const {
			return std::hash<GLint>()(coord.x) ^ std::hash<GLint>()(coord.z);
		}
	};

	const std::unordered_map<ChunkCoord, Chunk*, ChunkCoordHash>& getChunks() const { return chunks; }

private:
	struct ChunkCoordComparator {
		ChunkCoordComparator(const World& world) : world(world) {}
		bool operator()(const ChunkCoord& a, const ChunkCoord& b) const;
		const World& world;
	};

	void queueChunkLoad(GLint x, GLint z);
	void loadChunk(GLint x, GLint z);
	void unloadChunk(GLint x, GLint z);
	bool isWithinRenderDistance(GLint x, GLint z) const;
	bool isChunkLoaded(GLint x, GLint z) const;
	void updateNeighboringChunksOnBlockChange(GLint chunkX, GLint chunkZ, GLint localX, GLint localY, GLint localZ);
	bool isChunkInFrustum(GLint chunkX, GLint chunkZ, const Frustum& frustum) const;

	std::unordered_map<ChunkCoord, Chunk*, ChunkCoordHash> chunks;
	std::priority_queue<ChunkCoord, std::vector<ChunkCoord>, ChunkCoordComparator> chunkLoadQueue;
	GLint playerChunkX, playerChunkZ;
	TextureManager textureManager;

	std::unordered_map<ChunkCoord, std::future<Chunk*>, ChunkCoordHash> pendingChunks;
	std::mutex chunksMutex;
	ThreadPool threadPool;

	const uint8_t renderDistance = 15;

};