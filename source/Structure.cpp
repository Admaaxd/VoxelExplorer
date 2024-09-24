#include "Structure.h"
#include "Chunk.h"
#include "World.h"

void Structure::generateBaseTree(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z) {
    uint8_t treeHeight = 5 + rand() % 2;  // Trunk height

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