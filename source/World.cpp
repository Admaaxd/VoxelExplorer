#include "World.h"

World::World(const Frustum& frustum) : playerChunkX(0), playerChunkZ(0), chunkLoadQueue(ChunkCoordComparator(*this)), textureManager(), threadPool(std::thread::hardware_concurrency()) {
	std::srand(static_cast<GLuint>(std::time(0)));

	for (int8_t x = -renderDistance + 1; x <= renderDistance - 1; ++x)
	{
		for (int8_t z = -renderDistance + 1; z <= renderDistance - 1; ++z)
		{
			queueChunkLoad(x, z);
		}
	}
	processChunkLoadQueue(renderDistance * renderDistance, 0.5);
}

World::~World()
{
	std::lock_guard<std::mutex> lock(chunksMutex);
	for (auto& pair : chunks)
	{
		delete pair.second;
	}
}

bool World::isInitialChunksLoaded() {
	for (int8_t x = -renderDistance + 1; x <= renderDistance - 1; ++x) {
		for (int8_t z = -renderDistance + 1; z <= renderDistance - 1; ++z) {
			ChunkCoord coord = { x, z };
			if (!isChunkLoaded(coord.x, coord.z)) {
				return false;
			}
		}
	}
	return true;
}

void World::Draw(const Frustum& frustum) {
	std::vector<Chunk*> chunksToDraw, chunksNeedingUpdate;

	{
		std::lock_guard<std::mutex> lock(chunksMutex);
		for (auto& pair : chunks) {
			Chunk* chunk = pair.second;
			if (chunk) {
				if (chunk->needsMeshUpdate) {
					chunksNeedingUpdate.push_back(chunk);
				}
				if (chunk->isInFrustum(frustum)) {
					chunksToDraw.push_back(chunk);
				}
			}
		}
	}

	std::vector<std::future<void>> futures;
	for (Chunk* chunk : chunksNeedingUpdate) {
		futures.push_back(threadPool.enqueue([chunk]() {
			chunk->generateMesh(chunk->getBlockTypes());
			chunk->needsMeshUpdate = false;
		}));
	}

	for (auto& future : futures) future.get();

	for (Chunk* chunk : chunksToDraw) {
		chunk->updateOpenGLBuffers();
		chunk->Draw();
	}
}

void World::DrawWater(const Frustum& frustum, shader& waterShader, glm::mat4 view, glm::mat4 projection, glm::vec3 lightDirection, Camera& camera) {
	std::vector<Chunk*> chunksToDraw, chunksNeedingUpdate;

	{
		std::lock_guard<std::mutex> lock(chunksMutex);
		for (auto& pair : chunks) {
			Chunk* chunk = pair.second;
			if (chunk) {
				if (chunk->needsMeshUpdate) {
					if (chunk->isInFrustum(frustum)) {
						chunksNeedingUpdate.push_back(chunk);
					}
				}
				if (chunk->isInFrustum(frustum)) {
					chunksToDraw.push_back(chunk);
				}
			}
		}
	}

	std::vector<std::future<void>> futures;
	for (Chunk* chunk : chunksNeedingUpdate) {
		futures.push_back(threadPool.enqueue([chunk]() {
			chunk->generateMesh(chunk->getBlockTypes());
			chunk->needsMeshUpdate = false;
		}));
	}

	for (auto& future : futures) future.get();

	for (Chunk* chunk : chunksToDraw) {
		chunk->updateOpenGLWaterBuffers();
		chunk->DrawWater(waterShader, view, projection, lightDirection, camera);
	}
}

void World::updatePlayerPosition(const glm::vec3& position, const Frustum& frustum)
{
	int16_t newChunkX = static_cast<int16_t>(std::floor(position.x / CHUNK_SIZE));
	int16_t newChunkZ = static_cast<int16_t>(std::floor(position.z / CHUNK_SIZE));

	if (newChunkX != playerChunkX || newChunkZ != playerChunkZ)
	{
		playerChunkX = newChunkX;
		playerChunkZ = newChunkZ;

		for (int16_t dx = -renderDistance; dx <= renderDistance; ++dx)
		{
			for (int16_t dz = -renderDistance; dz <= renderDistance; ++dz)
			{
				int16_t chunkX = playerChunkX + dx;
				int16_t chunkZ = playerChunkZ + dz;

				if (isChunkInFrustum(chunkX, chunkZ, frustum) && !isChunkLoaded(chunkX, chunkZ))
				{
					queueChunkLoad(chunkX, chunkZ);
				}
			}
		}
	}
}

void World::processChunkLoadQueue(uint8_t maxChunksToLoad, uint16_t delay)
{
	static auto lastChunkLoadTime = std::chrono::steady_clock::now();
	uint8_t chunksLoaded = 0;
	std::vector<ChunkCoord> tempUnloadList;

	auto currentTime = std::chrono::steady_clock::now();
	auto timeSinceLastChunk = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastChunkLoadTime).count();

	while (!chunkLoadQueue.empty() && chunksLoaded < maxChunksToLoad) 
	{
		if (timeSinceLastChunk < delay)
			return;

		ChunkCoord coord = chunkLoadQueue.top();
		chunkLoadQueue.pop();

		if (isWithinRenderDistance(coord.x, coord.z) && !isChunkLoaded(coord.x, coord.z)) 
		{
			loadChunk(coord.x, coord.z);
			++chunksLoaded;

			lastChunkLoadTime = std::chrono::steady_clock::now();

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
			addChunk(chunk);
			it = pendingChunks.erase(it);
		}
		else {
			++it;
		}
	}

	for (const auto& coord : tempUnloadList) {
		unloadChunk(coord.x, coord.z);
	}
}

void World::addChunk(Chunk* chunk) {
	ChunkCoord coord = { chunk->getChunkX(), chunk->getChunkZ() };
	{
		std::lock_guard<std::mutex> lock(chunksMutex);
		chunks[coord] = chunk;
	}
	threadPool.enqueue([=] { notifyNeighbors(chunk); });
}

void World::notifyNeighbors(Chunk* chunk) {
	int16_t chunkX = chunk->getChunkX();
	int16_t chunkZ = chunk->getChunkZ();
	int16_t offsets[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };

	for (uint8_t i = 0; i < 4; ++i) {
		int16_t neighborX = chunkX + offsets[i][0];
		int16_t neighborZ = chunkZ + offsets[i][1];
		threadPool.enqueue([=]() {
			if (Chunk* neighbor = getChunk(neighborX, neighborZ)) {
				neighbor->needsMeshUpdate = true;
			}
		});
	}
}

void World::queueBlockChange(int16_t chunkX, int16_t chunkZ, int16_t localX, int16_t localY, int16_t localZ, uint8_t blockType) {
	std::lock_guard<std::mutex> lock(queuedBlockChangesMutex);
	ChunkCoord coord = { chunkX, chunkZ };
	queuedBlockChanges[coord].push_back({ localX, localY, localZ, blockType });
}

std::vector<BlockChange> World::getQueuedBlockChanges(int16_t chunkX, int16_t chunkZ) {
	std::lock_guard<std::mutex> lock(queuedBlockChangesMutex);
	ChunkCoord coord = { chunkX, chunkZ };
	std::vector<BlockChange> changes;
	auto it = queuedBlockChanges.find(coord);
	if (it != queuedBlockChanges.end()) {
		changes = it->second;
		queuedBlockChanges.erase(it);
	}
	return changes;
}

Chunk* World::getChunk(int16_t x, int16_t z)
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

void World::setBlock(int16_t x, int16_t y, int16_t z, int8_t type) {
	int16_t chunkX = x >= 0 ? x / CHUNK_SIZE : (x + 1) / CHUNK_SIZE - 1;
	int16_t chunkZ = z >= 0 ? z / CHUNK_SIZE : (z + 1) / CHUNK_SIZE - 1;

	uint8_t localX = (x % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
	uint8_t localY = y;
	uint8_t localZ = (z % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;

	Chunk* chunk = getChunk(chunkX, chunkZ);
	if (chunk) {
		threadPool.enqueue([this, chunk, localX, localY, localZ, type, chunkX, chunkZ]() {
			chunk->setBlockType(localX, localY, localZ, type);
			propagateSunlight(chunkX, chunkZ, localX, localY, localZ);
			chunk->needsMeshUpdate = true;

			if (localX == 0 || localX == CHUNK_SIZE - 1 ||
				localZ == 0 || localZ == CHUNK_SIZE - 1) {
				updateNeighboringChunksOnBlockChange(chunkX, chunkZ, localX, localY, localZ);
			}
		});
	}
}

void World::propagateSunlight(int16_t chunkX, int16_t chunkZ, int16_t localX, int16_t localY, int16_t localZ) {
	Chunk* chunk = getChunk(chunkX, chunkZ);
	if (chunk) {
		chunk->recalculateSunlightColumn(localX, localZ);
		chunk->needsMeshUpdate = true;
	}
}

void World::queueChunkLoad(int16_t x, int16_t z)
{
	ChunkCoord coord = { x, z };
	chunkLoadQueue.push(coord);
}

void World::loadChunk(int16_t x, int16_t z) {
	ChunkCoord coord = { x, z };
	if (chunks.find(coord) == chunks.end() && pendingChunks.find(coord) == pendingChunks.end()) {
		auto future = threadPool.enqueue([this, coord]() -> Chunk* {
			Chunk* chunk = new Chunk(coord.x, coord.z, textureManager, this);
			auto changes = getQueuedBlockChanges(coord.x, coord.z);
			for (const auto& change : changes) {
				chunk->setBlockType(change.localX, change.localY, change.localZ, change.blockType);
			}
			return chunk;
		});
		pendingChunks[coord] = std::move(future);
	}
}

void World::unloadChunk(int16_t x, int16_t z) {
	ChunkCoord coord = { x, z };
	std::unique_lock<std::mutex> lock(chunksMutex);
	auto it = chunks.find(coord);
	if (it != chunks.end()) {
		Chunk* chunk = it->second;
		chunks.erase(it);
		lock.unlock();

		delete chunk;
	}
}

bool World::isChunkLoaded(int16_t x, int16_t z) {
	ChunkCoord coord = { x, z };
	std::lock_guard<std::mutex> lock(chunksMutex);
	return chunks.find(coord) != chunks.end();
}

void World::updateNeighboringChunksOnBlockChange(int16_t chunkX, int16_t chunkZ, int16_t localX, int16_t localY, int16_t localZ) {
	std::vector<std::pair<int16_t, int16_t>> neighborsToUpdate;

	if (localX == 0) neighborsToUpdate.emplace_back(chunkX - 1, chunkZ);
	if (localX == CHUNK_SIZE - 1) neighborsToUpdate.emplace_back(chunkX + 1, chunkZ);
	if (localZ == 0) neighborsToUpdate.emplace_back(chunkX, chunkZ - 1);
	if (localZ == CHUNK_SIZE - 1) neighborsToUpdate.emplace_back(chunkX, chunkZ + 1);

	for (const auto& [nx, nz] : neighborsToUpdate) {
		threadPool.enqueue([this, nx, nz]() {
			Chunk* neighborChunk = getChunk(nx, nz);
			if (neighborChunk) {
				neighborChunk->needsMeshUpdate = true;
			}
		});
	}
}

bool World::isChunkInFrustum(int16_t chunkX, int16_t chunkZ, const Frustum& frustum) const
{
	// Calculate the chunk's AABB
	glm::vec3 minBounds(chunkX * CHUNK_SIZE, 0, chunkZ * CHUNK_SIZE);
	glm::vec3 maxBounds(chunkX * CHUNK_SIZE + CHUNK_SIZE, CHUNK_HEIGHT, chunkZ * CHUNK_SIZE + CHUNK_SIZE);
	return frustum.isBoxInFrustum(minBounds, maxBounds);
}

bool World::isWithinRenderDistance(int16_t x, int16_t z) const
{
	return std::abs(x - playerChunkX) <= renderDistance && std::abs(z - playerChunkZ) <= renderDistance;
}

bool World::ChunkCoordComparator::operator()(const ChunkCoord& a, const ChunkCoord& b) const {
    GLfloat distA = std::abs(a.x - world.playerChunkX) + std::abs(a.z - world.playerChunkZ);
    GLfloat distB = std::abs(b.x - world.playerChunkX) + std::abs(b.z - world.playerChunkZ);
    return distA > distB;
}

GLfloat World::getTerrainHeightAt(GLfloat x, GLfloat z)
{
	GLint chunkX = static_cast<GLint>(floor(x)) / CHUNK_SIZE;
	GLint chunkZ = static_cast<GLint>(floor(z)) / CHUNK_SIZE;

	Chunk* chunk = getChunk(chunkX, chunkZ);
	if (!chunk) return CHUNK_HEIGHT;

	GLint localX = static_cast<GLint>(floor(x)) % CHUNK_SIZE;
	GLint localZ = static_cast<GLint>(floor(z)) % CHUNK_SIZE;

	if (localX < 0) localX += CHUNK_SIZE;
	if (localZ < 0) localZ += CHUNK_SIZE;

	return static_cast<GLfloat>(chunk->getTerrainHeightAt(localX, localZ));
}