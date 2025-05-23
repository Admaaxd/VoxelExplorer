#include "Structure.h"
#include "Chunk.h"
#include "World.h"

/*
// Minecraft looking tree
void Structure::generateBaseTree(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z) {
    uint8_t treeHeight = 5 + rand() % 2;  // Random trunk height

    auto setLocalBlockType = [&](GLint offsetX, GLint offsetY, GLint offsetZ, uint8_t blockType) {
        int32_t newX = x + offsetX;
        int32_t newY = y + offsetY;
        int32_t newZ = z + offsetZ;

        int32_t globalX = chunk.chunkX * CHUNK_SIZE + newX;
        int32_t globalZ = chunk.chunkZ * CHUNK_SIZE + newZ;

        int32_t targetChunkX = globalX / CHUNK_SIZE;
        if (globalX < 0 && globalX % CHUNK_SIZE != 0) targetChunkX -= 1;

        int32_t targetChunkZ = globalZ / CHUNK_SIZE;
        if (globalZ < 0 && globalZ % CHUNK_SIZE != 0) targetChunkZ -= 1;

        int32_t localX = globalX - targetChunkX * CHUNK_SIZE;
        int32_t localZ = globalZ - targetChunkZ * CHUNK_SIZE;

        Chunk* targetChunk = chunk.world->getChunk(targetChunkX, targetChunkZ);

        if (targetChunk) {
            targetChunk->setBlockType(localX, newY, localZ, blockType);
            targetChunk->needsMeshUpdate = true;
        }
        else {
            // If the chunk is not loaded, queue the block change
            chunk.world->queueBlockChange(targetChunkX, targetChunkZ, localX, newY, localZ, blockType);
        }
    };

    // Trunk
    for (uint8_t i = 0; i < treeHeight; ++i) {
        setLocalBlockType(0, i, 0, 5); // Oak log
    }

    // Layer 3 and 4: full 3x3 square of leaves
    for (int8_t ly = treeHeight - 3; ly <= treeHeight - 2; ++ly) {
        for (int8_t lx = -2; lx <= 2; ++lx) {
            for (int8_t lz = -2; lz <= 2; ++lz) {
                if (!(lx == 0 && lz == 0) && !(lx == -2 && lz == -2 && ly == treeHeight - 3)) {
                    setLocalBlockType(lx, ly, lz, 6); // Oak leaves
                }
            }
        }
    }

    // Layer 5
    for (int8_t lx = -1; lx <= 1; ++lx) {
        for (int8_t lz = -1; lz <= 1; ++lz) {
            int8_t ly = treeHeight - 1;
            if ((lx == 0 && abs(lz) == 1) || (lz == 0 && abs(lx) == 1) || (lx == 1 && lz == 1) ||
                (lx == -1 && lz == 0) || (lx == 0 && lz == -1) || (lx == 1 && lz == 0)) {
                setLocalBlockType(lx, ly, lz, 6);
            }
        }
    }

    // Layer 6
    for (int8_t lx = -1; lx <= 1; ++lx) {
        for (int8_t lz = -1; lz <= 1; ++lz) {
            int8_t ly = treeHeight;
            if ((lx == 0 && abs(lz) == 1) || (lz == 0 && abs(lx) == 1)) {
                setLocalBlockType(lx, ly, lz, 6);
            }
        }
    }

    // Topmost leaf
    setLocalBlockType(0, treeHeight, 0, 6);
}
*/

void Structure::generateBaseProceduralTree(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z) {
    uint8_t treeHeight = 10 + rand() % 6;  // Random trunk height

    std::unordered_map<World::ChunkCoord, std::vector<BlockChange>, World::ChunkCoordHash> chunkBlockChanges;

    auto setLocalBlockType = [&](GLint offsetX, GLint offsetY, GLint offsetZ, uint8_t blockType) {
        int32_t newX = x + offsetX;
        int32_t newY = y + offsetY;
        int32_t newZ = z + offsetZ;

        int32_t globalX = chunk.chunkX * CHUNK_SIZE + newX;
        int32_t globalZ = chunk.chunkZ * CHUNK_SIZE + newZ;

        int32_t targetChunkX = globalX / CHUNK_SIZE;
        if (globalX < 0 && globalX % CHUNK_SIZE != 0) targetChunkX -= 1;

        int32_t targetChunkZ = globalZ / CHUNK_SIZE;
        if (globalZ < 0 && globalZ % CHUNK_SIZE != 0) targetChunkZ -= 1;

        int32_t localX = globalX - targetChunkX * CHUNK_SIZE;
        int32_t localZ = globalZ - targetChunkZ * CHUNK_SIZE;

        World::ChunkCoord targetCoord = { static_cast<int16_t>(targetChunkX), static_cast<int16_t>(targetChunkZ) };

        chunkBlockChanges[targetCoord].push_back({ static_cast<int16_t>(localX), static_cast<int16_t>(newY), static_cast<int16_t>(localZ), blockType });
    };

    for (uint8_t i = 0; i < treeHeight - 1; ++i) 
    {
        setLocalBlockType(0, i, 0, OAK_LOG);
    }

    // Place a leaf block at the top
    setLocalBlockType(0, treeHeight - 1, 0, OAK_LEAF);

    int8_t leafRadius = 3 + rand() % 2;
    int8_t leafStart = treeHeight - (leafRadius + 3);

    FastNoiseLite leafNoise;
    leafNoise.SetSeed(rand());
    leafNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    leafNoise.SetFrequency(1.0f);

    for (int8_t yOffset = 0; yOffset <= leafRadius + 2; ++yOffset) {
        int8_t layerY = leafStart + yOffset;
        int8_t radius = leafRadius - yOffset / 2;

        for (int8_t xOffset = -radius; xOffset <= radius; ++xOffset) {
            for (int8_t zOffset = -radius; zOffset <= radius; ++zOffset) {
                GLfloat distance = sqrtf(xOffset * xOffset + zOffset * zOffset);

                if (distance <= radius + 0.5f) {
                    GLfloat noiseValue = leafNoise.GetNoise
                    (
                        static_cast<GLfloat>(chunk.chunkX * CHUNK_SIZE + x + xOffset),
                        static_cast<GLfloat>(layerY),
                        static_cast<GLfloat>(chunk.chunkZ * CHUNK_SIZE + z + zOffset)
                    );

                    if (noiseValue > -0.5f) {
                        setLocalBlockType(xOffset, layerY, zOffset, OAK_LEAF);
                    }
                }
            }
        }
    }

    for (const auto& [coord, changes] : chunkBlockChanges) {
        Chunk* targetChunk = chunk.world->getChunk(coord.x, coord.z);
        if (targetChunk) {
            for (const auto& change : changes) {
                targetChunk->setBlockType(change.localX, change.localY, change.localZ, change.blockType);
            }
            targetChunk->needsMeshUpdate = true;
        }
        else {
            chunk.world->queueBlockChanges(coord.x, coord.z, changes);
        }
    }
}

void Structure::generateProceduralTreeOrangeLeaves(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z) {
    uint8_t treeHeight = 10 + rand() % 6;

    std::unordered_map<World::ChunkCoord, std::vector<BlockChange>, World::ChunkCoordHash> chunkBlockChanges;

    auto setLocalBlockType = [&](GLint offsetX, GLint offsetY, GLint offsetZ, uint8_t blockType) {
        int32_t newX = x + offsetX;
        int32_t newY = y + offsetY;
        int32_t newZ = z + offsetZ;

        int32_t globalX = chunk.chunkX * CHUNK_SIZE + newX;
        int32_t globalZ = chunk.chunkZ * CHUNK_SIZE + newZ;

        int32_t targetChunkX = globalX / CHUNK_SIZE;
        if (globalX < 0 && globalX % CHUNK_SIZE != 0) targetChunkX -= 1;

        int32_t targetChunkZ = globalZ / CHUNK_SIZE;
        if (globalZ < 0 && globalZ % CHUNK_SIZE != 0) targetChunkZ -= 1;

        int32_t localX = globalX - targetChunkX * CHUNK_SIZE;
        int32_t localZ = globalZ - targetChunkZ * CHUNK_SIZE;

        World::ChunkCoord targetCoord = { static_cast<int16_t>(targetChunkX), static_cast<int16_t>(targetChunkZ) };

        chunkBlockChanges[targetCoord].push_back({ static_cast<int16_t>(localX), static_cast<int16_t>(newY), static_cast<int16_t>(localZ), blockType });
    };

    for (uint8_t i = 0; i < treeHeight - 1; ++i) {
        setLocalBlockType(0, i, 0, OAK_LOG);
    }

    setLocalBlockType(0, treeHeight - 1, 0, OAK_LEAF_ORANGE);

    int8_t leafRadius = 3 + rand() % 2;
    int8_t leafStart = treeHeight - (leafRadius + 3);

    FastNoiseLite leafNoise;
    leafNoise.SetSeed(rand() * 2);
    leafNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    leafNoise.SetFrequency(1.5f);

    FastNoiseLite secondaryNoise;
    secondaryNoise.SetSeed(rand() * 3);
    secondaryNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    secondaryNoise.SetFrequency(0.75f);

    for (int8_t yOffset = 0; yOffset <= leafRadius + 2; ++yOffset) {
        int8_t layerY = leafStart + yOffset;
        int8_t radius = leafRadius - yOffset / 2;

        for (int8_t xOffset = -radius; xOffset <= radius; ++xOffset) {
            for (int8_t zOffset = -radius; zOffset <= radius; ++zOffset) {
                GLfloat distance = sqrtf(xOffset * xOffset + zOffset * zOffset);

                if (distance <= radius + 0.5f) {
                    GLfloat noiseValue = leafNoise.GetNoise(
                        static_cast<GLfloat>(chunk.chunkX * CHUNK_SIZE + x + xOffset),
                        static_cast<GLfloat>(layerY),
                        static_cast<GLfloat>(chunk.chunkZ * CHUNK_SIZE + z + zOffset)
                    );

                    GLfloat secondaryNoiseValue = secondaryNoise.GetNoise(
                        static_cast<GLfloat>(chunk.chunkX * CHUNK_SIZE + x + xOffset),
                        static_cast<GLfloat>(layerY),
                        static_cast<GLfloat>(chunk.chunkZ * CHUNK_SIZE + z + zOffset)
                    );

                    if (noiseValue > -0.5f && secondaryNoiseValue > -0.3f) {
                        setLocalBlockType(xOffset, layerY, zOffset, OAK_LEAF_ORANGE);
                    }
                }
            }
        }
    }

    for (const auto& [coord, changes] : chunkBlockChanges) {
        Chunk* targetChunk = chunk.world->getChunk(coord.x, coord.z);
        if (targetChunk) {
            for (const auto& change : changes) {
                targetChunk->setBlockType(change.localX, change.localY, change.localZ, change.blockType);
            }
            targetChunk->needsMeshUpdate = true;
        }
        else {
            chunk.world->queueBlockChanges(coord.x, coord.z, changes);
        }
    }
}

void Structure::generateProceduralTreeYellowLeaves(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z) {
    uint8_t treeHeight = 10 + rand() % 6;

    std::unordered_map<World::ChunkCoord, std::vector<BlockChange>, World::ChunkCoordHash> chunkBlockChanges;

    auto setLocalBlockType = [&](GLint offsetX, GLint offsetY, GLint offsetZ, uint8_t blockType) {
        int32_t newX = x + offsetX;
        int32_t newY = y + offsetY;
        int32_t newZ = z + offsetZ;

        int32_t globalX = chunk.chunkX * CHUNK_SIZE + newX;
        int32_t globalZ = chunk.chunkZ * CHUNK_SIZE + newZ;

        int32_t targetChunkX = globalX / CHUNK_SIZE;
        if (globalX < 0 && globalX % CHUNK_SIZE != 0) targetChunkX -= 1;

        int32_t targetChunkZ = globalZ / CHUNK_SIZE;
        if (globalZ < 0 && globalZ % CHUNK_SIZE != 0) targetChunkZ -= 1;

        int32_t localX = globalX - targetChunkX * CHUNK_SIZE;
        int32_t localZ = globalZ - targetChunkZ * CHUNK_SIZE;

        World::ChunkCoord targetCoord = { static_cast<int16_t>(targetChunkX), static_cast<int16_t>(targetChunkZ) };

        chunkBlockChanges[targetCoord].push_back({ static_cast<int16_t>(localX), static_cast<int16_t>(newY), static_cast<int16_t>(localZ), blockType });
    };

    for (uint8_t i = 0; i < treeHeight - 1; ++i) {
        setLocalBlockType(0, i, 0, OAK_LOG);
    }

    setLocalBlockType(0, treeHeight - 1, 0, OAK_LEAF_YELLOW);

    int8_t leafRadius = 3 + rand() % 2;
    int8_t leafStart = treeHeight - (leafRadius + 3);

    FastNoiseLite leafNoise;
    leafNoise.SetSeed(rand() * 5);
    leafNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    leafNoise.SetFrequency(2.0f);

    FastNoiseLite cellularNoise;
    cellularNoise.SetSeed(rand() * 7);
    cellularNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    cellularNoise.SetFrequency(0.7f);

    for (int8_t yOffset = 0; yOffset <= leafRadius + 2; ++yOffset) {
        int8_t layerY = leafStart + yOffset;
        int8_t radius = leafRadius - yOffset / 2;

        for (int8_t xOffset = -radius; xOffset <= radius; ++xOffset) {
            for (int8_t zOffset = -radius; zOffset <= radius; ++zOffset) {
                GLfloat distance = sqrtf(xOffset * xOffset + zOffset * zOffset);

                if (distance <= radius + 0.5f) {
                    GLfloat noiseValue = leafNoise.GetNoise(
                        static_cast<GLfloat>(chunk.chunkX * CHUNK_SIZE + x + xOffset),
                        static_cast<GLfloat>(layerY),
                        static_cast<GLfloat>(chunk.chunkZ * CHUNK_SIZE + z + zOffset)
                    );

                    GLfloat cellularNoiseValue = cellularNoise.GetNoise(
                        static_cast<GLfloat>(chunk.chunkX * CHUNK_SIZE + x + xOffset),
                        static_cast<GLfloat>(layerY),
                        static_cast<GLfloat>(chunk.chunkZ * CHUNK_SIZE + z + zOffset)
                    );

                    if (noiseValue > -0.2f && cellularNoiseValue < 0.3f) {
                        setLocalBlockType(xOffset, layerY, zOffset, OAK_LEAF_YELLOW);
                    }
                }
            }
        }
    }

    for (const auto& [coord, changes] : chunkBlockChanges) {
        Chunk* targetChunk = chunk.world->getChunk(coord.x, coord.z);
        if (targetChunk) {
            for (const auto& change : changes) {
                targetChunk->setBlockType(change.localX, change.localY, change.localZ, change.blockType);
            }
            targetChunk->needsMeshUpdate = true;
        }
        else {
            chunk.world->queueBlockChanges(coord.x, coord.z, changes);
        }
    }
}

void Structure::generateBasePurpleTree(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z)
{
    uint8_t trunkHeight = 3 + rand() % 2;
    uint8_t leafHeight = 2 + rand() % 2;

    std::unordered_map<World::ChunkCoord, std::vector<BlockChange>, World::ChunkCoordHash> chunkBlockChanges;

    auto setLocalBlockType = [&](GLint offsetX, GLint offsetY, GLint offsetZ, uint8_t blockType) {
        int32_t newX = x + offsetX;
        int32_t newY = y + offsetY;
        int32_t newZ = z + offsetZ;

        int32_t globalX = chunk.chunkX * CHUNK_SIZE + newX;
        int32_t globalZ = chunk.chunkZ * CHUNK_SIZE + newZ;

        int32_t targetChunkX = globalX / CHUNK_SIZE;
        if (globalX < 0 && globalX % CHUNK_SIZE != 0) targetChunkX -= 1;

        int32_t targetChunkZ = globalZ / CHUNK_SIZE;
        if (globalZ < 0 && globalZ % CHUNK_SIZE != 0) targetChunkZ -= 1;

        int32_t localX = globalX - targetChunkX * CHUNK_SIZE;
        int32_t localZ = globalZ - targetChunkZ * CHUNK_SIZE;

        World::ChunkCoord targetCoord = { static_cast<int16_t>(targetChunkX), static_cast<int16_t>(targetChunkZ) };

        chunkBlockChanges[targetCoord].push_back({ static_cast<int16_t>(localX), static_cast<int16_t>(newY), static_cast<int16_t>(localZ), blockType });
    };

    // Generate trunk
    for (uint8_t i = 0; i < trunkHeight; ++i) {
        setLocalBlockType(0, i, 0, OAK_LOG);
    }

    // Generate branches
    int8_t branchStart = trunkHeight - 1;
    for (int8_t i = 0; i < 4; ++i) {
        int8_t branchLength = 1 + rand() % 2;
        int8_t offsetX = (i % 2 == 0) ? branchLength : 0;
        int8_t offsetZ = (i % 2 == 1) ? branchLength : 0;

        for (int8_t j = 0; j < branchLength; ++j) {
            setLocalBlockType(offsetX - j, branchStart, offsetZ - j, OAK_LOG);
        }
    }

    for (uint8_t j = 0; j < leafHeight; ++j) {
        setLocalBlockType(0, trunkHeight + j, 0, OAK_LEAF_PURPLE);
    }

    for (GLint i = 0; i < 4; ++i) {
        int8_t offsetX = (i % 2 == 0) ? (1 + rand() % 2) : 0;
        int8_t offsetZ = (i % 2 == 1) ? (1 + rand() % 2) : 0;

        setLocalBlockType(offsetX, branchStart, offsetZ, OAK_LEAF_PURPLE);
    }

    for (const auto& [coord, changes] : chunkBlockChanges) {
        Chunk* targetChunk = chunk.world->getChunk(coord.x, coord.z);
        if (targetChunk) {
            for (const auto& change : changes) {
                targetChunk->setBlockType(change.localX, change.localY, change.localZ, change.blockType);
            }
            targetChunk->needsMeshUpdate = true;
        }
        else {
            chunk.world->queueBlockChanges(coord.x, coord.z, changes);
        }
    }
}
