#include "Biomes.h"

Biomes::Biomes(BiomeTypes type) : biomeTypes(type) {
    initializeNoise();
}

void Biomes::initializeNoise() {
    // Base noise
    baseNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    baseNoise.SetFractalType(FastNoiseLite::FractalType_FBm);

    // Elevation noise
    elevationNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    elevationNoise.SetFractalType(FastNoiseLite::FractalType_FBm);

    // Ridge noise
    ridgeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    ridgeNoise.SetFractalType(FastNoiseLite::FractalType_FBm);

    // Mountain noise
    mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    mountainNoise.SetFractalType(FastNoiseLite::FractalType_FBm);

    // Cliff noise
    cliffNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    cliffNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);

    // Detail noise
    detailNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    detailNoise.SetFractalType(FastNoiseLite::FractalType_FBm);

    // Tree noise
    treeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // Grass noise
    grassNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // Flower noise
    flowerNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    switch (biomeTypes) {
    case BiomeTypes::Forest:
        baseNoise.SetFrequency(0.005f);
        baseNoise.SetFractalOctaves(5);

        ridgeNoise.SetFrequency(0.0008f);
        ridgeNoise.SetFractalOctaves(5);

        mountainNoise.SetFrequency(0.001f);
        mountainNoise.SetFractalOctaves(7);
        
        detailNoise.SetFrequency(0.03f);
        detailNoise.SetFractalOctaves(10);

        treeNoise.SetFrequency(0.2f);

        grassNoise.SetSeed(10);
        grassNoise.SetFrequency(0.9f);

        flowerNoise.SetSeed(11);
        flowerNoise.SetFrequency(0.9f);

        break;

    case BiomeTypes::Desert:
        baseNoise.SetFrequency(0.0011f);
        baseNoise.SetFractalOctaves(2);

        cliffNoise.SetFrequency(0.005f);
        cliffNoise.SetFractalOctaves(3);
        cliffAmplitude = 0.5f;

        flowerNoise.SetSeed(11);
        flowerNoise.SetFrequency(0.9f);

        break;

    case BiomeTypes::Plains:
        baseNoise.SetFrequency(0.003f);
        baseNoise.SetFractalOctaves(3);

        detailNoise.SetFrequency(0.02f);
        detailNoise.SetFractalOctaves(5);

        treeNoise.SetFrequency(0.15f);

        grassNoise.SetSeed(10);
        grassNoise.SetFrequency(0.8f);

        flowerNoise.SetSeed(11);
        flowerNoise.SetFrequency(0.8f);

        break;

    case BiomeTypes::Mountain:
        baseNoise.SetFrequency(0.001f);
        baseNoise.SetFractalOctaves(4);

        cliffNoise.SetFrequency(0.003f);
        cliffNoise.SetFractalOctaves(9);
        cliffAmplitude = 1.0f;
    }

}

GLint Biomes::getTerrainHeightAt(GLint x, GLint z) const {
    GLfloat height = 0.0f;

    switch (biomeTypes) {
    case BiomeTypes::Forest:
        height = baseNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z));
        break;

    case BiomeTypes::Desert: {
            height = baseNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z)) * 0.5f;
            GLfloat cliffValue = cliffNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z));

            height += fabs(cliffValue) * cliffAmplitude;
            break;
        }
    case BiomeTypes::Plains:
        height = baseNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z)) * 0.6f;
        break;

    case BiomeTypes::Mountain: {
            height = baseNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z));
            GLfloat ridge = ridgeNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z));
            GLfloat cliffValue = cliffNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z));
            height += ridge + fabs(cliffValue) * cliffAmplitude;
            break;
        }
    }

    return static_cast<GLint>((height + 1.0f) * 0.5f * 128);
}

bool Biomes::isCave(GLint x, GLint y, GLint z) const {
    GLfloat caveValue = caveNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(y), static_cast<GLfloat>(z));
    return caveValue > 0.35f;
}

bool Biomes::shouldPlaceTree(GLint x, GLint z) const {
    GLfloat treeChance = treeNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z));

    if (biomeTypes == BiomeTypes::Forest) {
        return treeChance > 0.92f;
    }
    else if (biomeTypes == BiomeTypes::Plains) {
        return treeChance > 0.97f;
    }
    return false;
}

bool Biomes::shouldPlaceGrass(GLint x, GLint z) const {
    GLfloat grassChance = grassNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z));

    if (biomeTypes == BiomeTypes::Forest) {
        return grassChance > 0.45f;
    }
    else if (biomeTypes == BiomeTypes::Plains) {
        return grassChance > 0.55f;
    }
    return false;
}

bool Biomes::shouldPlaceFlower(GLint x, GLint z) const {
    GLfloat flowerChance = flowerNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z));

    if (biomeTypes == BiomeTypes::Forest) {
        return flowerChance > 0.70f;
    }
    else if (biomeTypes == BiomeTypes::Plains) {
        return flowerChance > 0.80f;
    }
    return false;
}

bool Biomes::shouldPlaceDeadBush(int x, int z) const {
    if (biomeTypes == BiomeTypes::Desert) {
        GLfloat deadBushChance = flowerNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z));
        return deadBushChance > 0.90f;
    }
    return false;
}

GLint Biomes::getRandomGrassType() const {
    if (biomeTypes == BiomeTypes::Forest || biomeTypes == BiomeTypes::Plains) {
        GLfloat randomValue = static_cast<GLfloat>(rand()) / RAND_MAX;
        if      (randomValue < 0.33f)   return GRASS1;
        else if (randomValue < 0.67f)   return GRASS2;
        else                            return GRASS3;
    }
    return -1;
}

GLint Biomes::getRandomFlowerType() const {
    if (biomeTypes == BiomeTypes::Forest || biomeTypes == BiomeTypes::Plains) {
        GLint flowerType = rand() % 5;
        switch (flowerType) {
        case 0: return FLOWER1;
        case 1: return FLOWER2;
        case 2: return FLOWER3;
        case 3: return FLOWER4;
        case 4: return FLOWER5;
        default: return -1;
        }
    }
    return -1;
}

GLint Biomes::getSurfaceBlock(GLint y) const {
    switch (biomeTypes) {
    case BiomeTypes::Forest:
        return GRASS_BLOCK;
    case BiomeTypes::Desert:
        return SAND;
    case BiomeTypes::Plains:
        return GRASS_BLOCK;
    case BiomeTypes::Mountain:
        return y >= 100 ? SNOW : STONE;
    }
    return DIRT;
}

GLint Biomes::getSubSurfaceBlock() const {
    switch (biomeTypes) {
    case BiomeTypes::Forest:
        return DIRT;
    case BiomeTypes::Desert:
        return SAND;
    case BiomeTypes::Plains:
        return DIRT;
    case BiomeTypes::Mountain:
        return STONE;
    }
    return STONE;
}

GLint Biomes::getUndergroundBlock() const {
    return STONE;
}