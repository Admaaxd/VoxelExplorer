#include "World.h"

World::World(const Frustum& frustum) : playerChunkX(0), playerChunkZ(0), chunkLoadQueue(ChunkCoordComparator(*this)), textureManager(), threadPool(std::thread::hardware_concurrency()) {
	std::srand(static_cast<GLuint>(std::time(0)));

	for (GLint x = -renderDistance; x <= renderDistance; ++x)
	{
		for (GLint z = -renderDistance; z <= renderDistance; ++z)
		{
			queueChunkLoad(x, z);
		}
	}
	processChunkLoadQueue(renderDistance * renderDistance);
}

World::~World()
{
	for (auto& pair : chunks)
	{
		delete pair.second;
	}
}

void World::Draw(const Frustum& frustum) {
	for (auto& pair : chunks) {
		Chunk* chunk = pair.second;
		if (chunk && chunk->isInFrustum(frustum)) {
			chunk->Draw();
			chunk->updateOpenGLBuffers();
		}
	}
}

void World::updatePlayerPosition(const glm::vec3& position)
{
	GLint newChunkX = static_cast<GLint>(std::floor(position.x / static_cast<GLint>(CHUNK_SIZE)));
	GLint newChunkZ = static_cast<GLint>(std::floor(position.z / static_cast<GLint>(CHUNK_SIZE)));

	if (newChunkX != playerChunkX || newChunkZ != playerChunkZ)
	{
		playerChunkX = newChunkX;
		playerChunkZ = newChunkZ;

		for (GLint dx = -renderDistance; dx <= renderDistance; ++dx)
		{
			for (GLint dz = -renderDistance; dz <= renderDistance; ++dz)
			{
				GLint chunkX = playerChunkX + dx;
				GLint chunkZ = playerChunkZ + dz;

				queueChunkLoad(chunkX, chunkZ);
			}
		}
	}
}

void World::processChunkLoadQueue(uint8_t maxChunksToLoad)
{
	GLint chunksLoaded = 0;
	std::vector<ChunkCoord> tempUnloadList;

	while (!chunkLoadQueue.empty() && chunksLoaded < maxChunksToLoad)
	{
		ChunkCoord coord = chunkLoadQueue.top();
		chunkLoadQueue.pop();

		if (isWithinRenderDistance(coord.x, coord.z) && !isChunkLoaded(coord.x, coord.z))
		{
			loadChunk(coord.x, coord.z);
			++chunksLoaded;

			for (const auto& pair : chunks)
			{
				const ChunkCoord& loadedCoord = pair.first;
				if (!isWithinRenderDistance(loadedCoord.x, loadedCoord.z))
				{
					tempUnloadList.push_back(loadedCoord);
				}
			}
		}
	}

	for (auto it = pendingChunks.begin(); it != pendingChunks.end(); )
	{
		if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			Chunk* chunk = it->second.get();
			{
				std::lock_guard<std::mutex> lock(chunksMutex);
				chunks[it->first] = chunk;
			}
			it = pendingChunks.erase(it);
		}
		else {
			++it;
		}
	}

	// Unload chunks marked for unloading
	for (const auto& coord : tempUnloadList) {
		unloadChunk(coord.x, coord.z);
	}
}

Chunk* World::getChunk(GLint x, GLint z)
{
	ChunkCoord coord = { x, z };
	std::lock_guard<std::mutex> lock(chunksMutex);
	auto it = chunks.find(coord);
	if (it != chunks.end())
	{
		return it->second;
	}
	return nullptr;
}

void World::setBlock(GLint x, GLint y, GLint z, int8_t type) {
	GLint chunkX = (x >= 0) ? x / CHUNK_SIZE : (x + 1) / CHUNK_SIZE - 1;
	GLint chunkZ = (z >= 0) ? z / CHUNK_SIZE : (z + 1) / CHUNK_SIZE - 1;

	GLint localX = (x % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
	GLint localY = y;
	GLint localZ = (z % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;

	if (Chunk* chunk = getChunk(chunkX, chunkZ)) {
		chunk->setBlockType(localX, localY, localZ, type);
		propagateSunlight(chunkX, chunkZ, localX, localY, localZ);
		chunk->generateMesh(chunk->getBlockTypes());
		chunk->updateOpenGLBuffers();
	}

	updateNeighboringChunksOnBlockChange(chunkX, chunkZ, localX, localY, localZ);
}

void World::propagateSunlight(GLint chunkX, GLint chunkZ, GLint localX, GLint localY, GLint localZ) {
	Chunk* chunk = getChunk(chunkX, chunkZ);
	if (!chunk) return;

	chunk->recalculateSunlightColumn(localX, localZ);
}

void World::queueChunkLoad(GLint x, GLint z)
{
	ChunkCoord coord = { x, z };
	chunkLoadQueue.push(coord);
}

void World::loadChunk(GLint x, GLint z) {
	ChunkCoord coord = { x, z };

	if (chunks.find(coord) == chunks.end() && pendingChunks.find(coord) == pendingChunks.end()) {
		auto future = threadPool.enqueue([this, coord, x, z]() -> Chunk* {
			Chunk* chunk = new Chunk(x, z, textureManager, this);

			std::lock_guard<std::mutex> lock(chunksMutex);
			chunks[coord] = chunk;
			return chunk;
		});

		pendingChunks[coord] = std::move(future);
	}
}

void World::unloadChunk(GLint x, GLint z)
{
	ChunkCoord coord = { x, z };
	std::lock_guard<std::mutex> lock(chunksMutex);
	auto it = chunks.find(coord);
	if (it != chunks.end())
	{
		delete it->second;
		chunks.erase(it);
	}
}

bool World::isChunkLoaded(GLint x, GLint z) const
{
	ChunkCoord coord = { x, z };
	return chunks.find(coord) != chunks.end();
}

void World::updateNeighboringChunksOnBlockChange(GLint chunkX, GLint chunkZ, GLint localX, GLint localY, GLint localZ) {
	if (localX == 0) {
		if (Chunk* neighborChunk = getChunk(chunkX - 1, chunkZ)) {
			neighborChunk->generateMesh(neighborChunk->getBlockTypes());
			neighborChunk->updateOpenGLBuffers();
		}
	}
	if (localX == CHUNK_SIZE - 1) {
		if (Chunk* neighborChunk = getChunk(chunkX + 1, chunkZ)) {
			neighborChunk->generateMesh(neighborChunk->getBlockTypes());
			neighborChunk->updateOpenGLBuffers();
		}
	}
	if (localZ == 0) {
		if (Chunk* neighborChunk = getChunk(chunkX, chunkZ - 1)) {
			neighborChunk->generateMesh(neighborChunk->getBlockTypes());
			neighborChunk->updateOpenGLBuffers();
		}
	}
	if (localZ == CHUNK_SIZE - 1) {
		if (Chunk* neighborChunk = getChunk(chunkX, chunkZ + 1)) {
			neighborChunk->generateMesh(neighborChunk->getBlockTypes());
			neighborChunk->updateOpenGLBuffers();
		}
	}
}

bool World::isChunkInFrustum(GLint chunkX, GLint chunkZ, const Frustum& frustum) const
{
	// Calculate the chunk's AABB
	glm::vec3 minBounds(chunkX * CHUNK_SIZE, 0, chunkZ * CHUNK_SIZE);
	glm::vec3 maxBounds(chunkX * CHUNK_SIZE + CHUNK_SIZE, CHUNK_HEIGHT, chunkZ * CHUNK_SIZE + CHUNK_SIZE);

	return frustum.isBoxInFrustum(minBounds, maxBounds);
}

bool World::isWithinRenderDistance(GLint x, GLint z) const
{
	return std::abs(x - playerChunkX) <= renderDistance && std::abs(z - playerChunkZ) <= renderDistance;
}

bool World::ChunkCoordComparator::operator()(const ChunkCoord& a, const ChunkCoord& b) const
{
	auto distA = std::sqrt(std::pow(a.x - world.playerChunkX, 2) + std::pow(a.z - world.playerChunkZ, 2));
	auto distB = std::sqrt(std::pow(b.x - world.playerChunkX, 2) + std::pow(b.z - world.playerChunkZ, 2));
	return distA > distB;
}