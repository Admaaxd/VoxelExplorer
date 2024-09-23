#include "Structure.h"
#include "Chunk.h"

void Structure::generateBaseTree(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z) {
    uint8_t treeHeight = 5 + rand() % 2;  // Trunk height

    if (y + treeHeight + 3 >= CHUNK_HEIGHT || x - 2 < 0 || x + 2 >= CHUNK_SIZE || z - 2 < 0 || z + 2 >= CHUNK_SIZE) {
       return; // Temporarily solution :(
    }

    auto setLocalBlockType = [&](GLint offsetX, GLint offsetY, GLint offsetZ, uint8_t blockType) {
        int8_t newX = x + offsetX;
        int8_t newY = y + offsetY;
        int8_t newZ = z + offsetZ;
        if (newX >= 0 && newX < CHUNK_SIZE && newY >= 0 && newY < CHUNK_HEIGHT && newZ >= 0 && newZ < CHUNK_SIZE) {
            chunk.setBlockType(newX, newY, newZ, blockType);
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