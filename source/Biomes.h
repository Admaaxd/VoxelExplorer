#pragma once

#include "FastNoiseLite.h"
#include "BiomeTypes.h"
#include "BlockTypes.h"
#include <glad/glad.h>
#include <cstdlib>

class Biomes {
public:
    explicit Biomes(BiomeTypes type);

    GLint getTerrainHeightAt(GLint x, GLint z) const;
    bool isCave(GLint x, GLint y, GLint z) const;

    bool shouldPlaceTree(GLint x, GLint z) const;
    bool shouldPlaceGrass(GLint x, GLint z) const;
    bool shouldPlaceFlower(GLint x, GLint z) const;
    bool shouldPlaceDeadBush(int x, int z) const;

    GLint getRandomGrassType() const;
    GLint getRandomFlowerType() const;

    GLint getSurfaceBlock() const;
    GLint getSubSurfaceBlock() const;
    GLint getUndergroundBlock() const;

private:
    void initializeNoise();

    BiomeTypes biomeTypes;

    FastNoiseLite baseNoise, elevationNoise, caveNoise, ridgeNoise, detailNoise, mountainNoise, treeNoise, grassNoise, flowerNoise;
};