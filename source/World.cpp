#include "World.h"

World::World() : playerChunkX(0), playerChunkZ(0), chunkLoadQueue(ChunkCoordComparator(*this)), textureManager() {
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

void World::Draw()
{
	for (auto& pair : chunks) {
		Chunk* chunk = pair.second;
		chunk->Draw();
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

		if (!isChunkLoaded(coord.x, coord.z))
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

			for (const auto& coord : tempUnloadList)
			{
				unloadChunk(coord.x, coord.z);
			}
			tempUnloadList.clear();
		}
	}
}

void World::queueChunkLoad(GLint x, GLint z)
{
	ChunkCoord coord = { x, z };
	chunkLoadQueue.push(coord);
}

void World::loadChunk(GLint x, GLint z)
{
	ChunkCoord coord = { x, z };
	if (chunks.find(coord) == chunks.end())
	{
		chunks[coord] = new Chunk(x, z, textureManager);
	}
}

void World::unloadChunk(GLint x, GLint z)
{
	ChunkCoord coord = { x, z };
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