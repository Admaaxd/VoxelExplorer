#include "Chunk.h"
#include "World.h"

Chunk::Chunk(GLint x, GLint z, TextureManager& textureManager, World* world)
    : chunkX(x), chunkZ(z), textureManager(textureManager), textureID(textureManager.getTextureID()), world(world)
{
    minBounds = glm::vec3(chunkX * CHUNK_SIZE, 0, chunkZ * CHUNK_SIZE);
    maxBounds = glm::vec3((chunkX + 1) * CHUNK_SIZE, CHUNK_HEIGHT, (chunkZ + 1) * CHUNK_SIZE);

    setupChunk();
    calculateBounds();
}

Chunk::~Chunk()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glDeleteVertexArrays(1, &waterVAO);
    glDeleteBuffers(1, &waterVBO);
    glDeleteBuffers(1, &waterEBO);
}

void Chunk::setupChunk()
{
    initializeNoise();
    generateChunk();
    generateMesh(blockTypes);
}

void Chunk::generateChunk()
{
    blockTypes.resize(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE, -1);

    constexpr uint8_t caveMinHeight = CHUNK_HEIGHT / 64;
    constexpr uint8_t surfaceBuffer = CHUNK_HEIGHT / 9;

    for (uint8_t x = 0; x < CHUNK_SIZE; ++x)
    {
        for (uint8_t z = 0; z < CHUNK_SIZE; ++z)
        {
            GLfloat globalX = static_cast<GLfloat>(chunkX * CHUNK_SIZE + x);
            GLfloat globalZ = static_cast<GLfloat>(chunkZ * CHUNK_SIZE + z);

            uint8_t terrainHeight = getTerrainHeightAt(globalX, globalZ);

            for (uint8_t y = 0; y < CHUNK_HEIGHT; ++y)
            {
                GLuint index = getIndex(x, y, z);

                // cave noise
                GLfloat caveValue = caveNoise.GetNoise(globalX, static_cast<GLfloat>(y), globalZ);

                // Vary cave thresholds based on height and tunnel values for complexity
                bool isMainCave = caveValue > 0.35f && y > caveMinHeight && y < (CHUNK_HEIGHT - surfaceBuffer);
                bool isTunnel = caveValue > 0.30f && y > caveMinHeight && y < (CHUNK_HEIGHT - surfaceBuffer - 20);

                if (isMainCave || isTunnel)
                {
                    blockTypes[index] = -1; // Air block for caves and tunnels
                    continue;
                }

                if (y > terrainHeight)
                {
                    if (y <= WATERLEVEL)
                    {
                        blockTypes[index] = 4; // Water
                    }
                    continue;
                }

                if (y <= WATERLEVEL && y >= terrainHeight - 2)
                {
                    blockTypes[index] = 3; // Sand near water
                }
                else if (y == terrainHeight)
                {
                    blockTypes[index] = 2; // Grass top
                }
                else if (y >= terrainHeight - 4)
                {
                    blockTypes[index] = 0; // Dirt layer
                }
                else if (y < terrainHeight - 4 && y > WATERLEVEL - 5)
                {
                    blockTypes[index] = 1; // Stone layer
                }
                else
                {
                    blockTypes[index] = 1; // Stone layer
                }
            }
        }
    }

    // Generate trees
    for (uint8_t x = 0; x < CHUNK_SIZE; ++x)
    {
        for (uint8_t z = 0; z < CHUNK_SIZE; ++z)
        {
            GLint globalX = chunkX * CHUNK_SIZE + x;
            GLint globalZ = chunkZ * CHUNK_SIZE + z;

            GLfloat treeValue = treeNoise.GetNoise((GLfloat)globalX, (GLfloat)globalZ);
            if (treeValue > 0.95f) // Threshold for tree placement
            {
                GLint terrainHeight = getTerrainHeightAt(globalX, globalZ);

                if (terrainHeight > WATERLEVEL + 1 && terrainHeight < CHUNK_HEIGHT - 15)
                {
                    GLint index = getIndex(x, terrainHeight, z);
                    if (blockTypes[index] == 2) // Grass block
                    {
                        //Structure::generateBaseTree(*this, x, terrainHeight + 1, z);
                        Structure::generateBaseProceduralTree(*this, x, terrainHeight + 1, z);
                    }
                }
            }
        }
    }

    // Plants
    for (uint8_t x = 0; x < CHUNK_SIZE; ++x)
    {
        for (uint8_t z = 0; z < CHUNK_SIZE; ++z)
        {
            GLint globalX = chunkX * CHUNK_SIZE + x;
            GLint globalZ = chunkZ * CHUNK_SIZE + z;

            uint8_t terrainHeight = getTerrainHeightAt(globalX, globalZ);

            GLint indexBelow = getIndex(x, terrainHeight, z);
            GLint indexAbove = getIndex(x, terrainHeight + 1, z);

            if (blockTypes[indexBelow] == 2 && blockTypes[indexAbove] == -1)
            {
                GLfloat grassChance = grassNoise.GetNoise((GLfloat)globalX, (GLfloat)globalZ);
                GLfloat flowerChance = flowerNoise.GetNoise((GLfloat)globalX, (GLfloat)globalZ);

                if (grassChance > 0.5f && grassChance > flowerChance)
                {
                    // Place grass
                    GLint grassType;
                    GLfloat randomValue = static_cast<GLfloat>(rand()) / RAND_MAX;
                    if (randomValue < 0.33f)
                        grassType = 11; // Grass 1
                    else if (randomValue < 0.67f)
                        grassType = 12; // Grass 2
                    else
                        grassType = 13; // Grass 3

                    blockTypes[indexAbove] = grassType;
                }
                else if (flowerChance > 0.90f)
                {
                    blockTypes[indexAbove] = 10; // Flower 1
                }
                // Else, other flowers in the future
            }
        }
    }

    lightLevels.resize(blockTypes.size(), 0);

    // Calculate light levels
    for (uint8_t x = 0; x < CHUNK_SIZE; ++x)
    {
        for (uint8_t z = 0; z < CHUNK_SIZE; ++z)
        {
            recalculateSunlightColumn(x, z);
        }
    }
    isInitialized = true;
}

void Chunk::initializeNoise()
{
    // Base noise
    baseNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    baseNoise.SetFrequency(0.005f);

    // Elevation noise (general hills and plains)
    elevationNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    elevationNoise.SetFrequency(0.001f);
    elevationNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    elevationNoise.SetFractalOctaves(7);

    // Ridge noise (sharp peaks and valleys)
    ridgeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    ridgeNoise.SetFrequency(0.0008f);
    ridgeNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    ridgeNoise.SetFractalOctaves(5);

    // Mountain noise
    mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    mountainNoise.SetFrequency(0.0001f);
    mountainNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    mountainNoise.SetFractalOctaves(7);

    // Detail noise
    detailNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    detailNoise.SetFrequency(0.03f);
    detailNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    detailNoise.SetFractalOctaves(10);

    // Cave noise
    caveNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    caveNoise.SetFrequency(0.009f);
    caveNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    caveNoise.SetFractalOctaves(4);

    // Tree noise
    treeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    treeNoise.SetFrequency(0.2f);

    // Grass noise
    grassNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    grassNoise.SetSeed(10);
    grassNoise.SetFrequency(0.9f);

    // Flower noise
    flowerNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    flowerNoise.SetSeed(10 + 1);
    flowerNoise.SetFrequency(0.9f);
}

GLint Chunk::getTerrainHeightAt(GLint x, GLint z)
{
    GLfloat finalHeight = baseNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z))
        + elevationNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z)) * 0.5f
        + ridgeNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z)) * 0.4f
        + mountainNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z)) * 0.5f
        + detailNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z)) * 0.2f;

    return static_cast<GLint>((finalHeight + 1.0f) * 0.5f * (CHUNK_HEIGHT - 30)) + 30;
}

void Chunk::placeBlockIfInChunk(GLint globalX, GLint y, GLint globalZ, GLint blockType)
{
    if (y >= 0 && y < CHUNK_HEIGHT) {
        GLint localX = globalX - chunkX * CHUNK_SIZE;
        GLint localZ = globalZ - chunkZ * CHUNK_SIZE;

        if (localX >= 0 && localX < CHUNK_SIZE && localZ >= 0 && localZ < CHUNK_SIZE) {
            blockTypes[getIndex(localX, y, localZ)] = blockType;
        }
    }
}

inline GLint Chunk::getIndex(GLint x, GLint y, GLint z) const
{
    static constexpr uint16_t layerSize = CHUNK_HEIGHT * CHUNK_SIZE;
    return x * layerSize + y * CHUNK_SIZE + z;
}

inline bool Chunk::isTransparent(GLint blockType)
{
    return blockType == -1 || blockType == 4 || blockType == 6 || blockType == 9 ||
        blockType == 10 || blockType == 11 || blockType == 12 || blockType == 13;
}

void Chunk::recalculateSunlightColumn(GLint x, GLint z) {
    uint8_t lightLevel = 15; // Maximum sunlight

    // Loop through the column from top to bottom
    for (GLint y = CHUNK_HEIGHT - 1; y >= 0; --y) {
        GLuint index = getIndex(x, y, z);

        if (isTransparent(blockTypes[index])) {
            lightLevels[index] = lightLevel;
        }
        else { // Solid block
            lightLevels[index] = lightLevel;
            lightLevel = 0; // Stop sunlight propagation at solid blocks
        }
    }
}

void Chunk::generateMesh(const std::vector<GLint>& blockTypes)
{
    vertices.clear();
    indices.clear();
    waterVertices.clear();
    waterIndices.clear();
    GLint vertexOffset = 0;
    GLint waterVertexOffset = 0;

    enum FaceFlag {
        BACK = 1 << 0,
        FRONT = 1 << 1,
        LEFT = 1 << 2,
        RIGHT = 1 << 3,
        TOP = 1 << 4,
        BOTTOM = 1 << 5,
    };

    struct ProcessedFaces {
        uint8_t flags = 0;
    };

    auto isFaceProcessed = [](const ProcessedFaces& processed, FaceFlag face) {
        return (processed.flags & face) != 0;
    };

    auto setFaceProcessed = [](ProcessedFaces& processed, FaceFlag face) {
        processed.flags |= face;
    };

    std::vector<std::vector<std::vector<ProcessedFaces>>> processed(
        CHUNK_SIZE, std::vector<std::vector<ProcessedFaces>>(CHUNK_HEIGHT, std::vector<ProcessedFaces>(CHUNK_SIZE)));

    auto isAir = [](GLint blockType) {
        return blockType == -1;
    };

    auto isExposed = [&](GLint x, GLint y, GLint z, GLint dx, GLint dy, GLint dz, GLint blockType) {
        GLint nx = x + dx, ny = y + dy, nz = z + dz;
        GLint neighborBlockType = -1; // Default to air

        if (nx >= 0 && ny >= 0 && nz >= 0 && nx < CHUNK_SIZE && ny < CHUNK_HEIGHT && nz < CHUNK_SIZE) {
            neighborBlockType = blockTypes[getIndex(nx, ny, nz)];
        }
        else {
            GLint neighborChunkX = chunkX, neighborChunkZ = chunkZ;
            GLint neighborX = nx, neighborZ = nz;

            if (nx < 0) {
                neighborChunkX -= 1;
                neighborX += CHUNK_SIZE;
            }
            else if (nx >= CHUNK_SIZE) {
                neighborChunkX += 1;
                neighborX -= CHUNK_SIZE;
            }

            if (nz < 0) {
                neighborChunkZ -= 1;
                neighborZ += CHUNK_SIZE;
            }
            else if (nz >= CHUNK_SIZE) {
                neighborChunkZ += 1;
                neighborZ -= CHUNK_SIZE;
            }

            Chunk* neighborChunk = world->getChunk(neighborChunkX, neighborChunkZ);
            if (neighborChunk) {
                neighborBlockType = neighborChunk->getBlockType(neighborX, ny, neighborZ);
            }
            else {
                neighborBlockType = -1;
            }
        }

        if (neighborBlockType == blockType) {
            return false; // Same block type; do not expose face
        }

        return isTransparent(neighborBlockType);
    };

    auto calculateAO = [&](bool side1, bool side2, bool corner) {
        GLfloat baseAO = 1.0f;
        if (side1 && side2) baseAO = 1.0f;
        else baseAO = 1.0f - (side1 + side2 + corner) / 3.0f;
        return glm::clamp(baseAO, 0.90f, 1.0f);
    };

    auto processFace = [&](GLint x, GLint y, GLint z, int8_t face) {
        uint16_t extentX = 1;
        uint16_t extentY = 1;
        GLint blockType = blockTypes[getIndex(x, y, z)];
        GLint textureLayer = getTextureLayer(blockType, face);
        GLuint index = getIndex(x, y, z);
        uint8_t lightLevel = lightLevels[index];
        GLfloat ao[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        switch (face) {
        case 0: // Back face

            ao[0] = calculateAO(isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, -1, 1, 0, blockType));
            ao[1] = calculateAO(isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 1, 1, 0, blockType));
            ao[2] = calculateAO(isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, -1, -1, 0, blockType));
            ao[3] = calculateAO(isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 1, -1, 0, blockType));

            while (y + extentY < CHUNK_HEIGHT && blockTypes[getIndex(x, y + extentY, z)] == blockType &&
                !isFaceProcessed(processed[x][y + extentY][z], FaceFlag::BACK) && 
                isExposed(x, y + extentY, z, 0, 0, -1, blockType)) {
                setFaceProcessed(processed[x][y + extentY][z], FaceFlag::BACK);
                extentY++;
            }

            extentX = 1;
            while (x + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (int16_t dy = 0; dy < extentY; ++dy) {
                    if (blockTypes[getIndex(x + extentX, y + dy, z)] != blockType ||
                        isFaceProcessed(processed[x + extentX][y + dy][z], FaceFlag::BACK) ||
                        !isExposed(x + extentX, y + dy, z, 0, 0, -1, blockType)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (int16_t dy = 0; dy < extentY; ++dy) {
                    setFaceProcessed(processed[x + extentX][y + dy][z], FaceFlag::BACK);
                }
                extentX++;
            }
            Block::addBackFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;

        case 1: // Front face

            ao[0] = calculateAO(isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, -1, 1, 1, blockType));
            ao[1] = calculateAO(isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 1, 1, 1, blockType));
            ao[2] = calculateAO(isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, -1, -1, 1, blockType));
            ao[3] = calculateAO(isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 1, -1, 1, blockType));

            while (y + extentY < CHUNK_HEIGHT && blockTypes[getIndex(x, y + extentY, z)] == blockType &&
                !isFaceProcessed(processed[x][y + extentY][z], FaceFlag::FRONT) &&
                isExposed(x, y + extentY, z, 0, 0, 1, blockType)) {
                setFaceProcessed(processed[x][y + extentY][z], FaceFlag::FRONT);
                extentY++;
            }

            extentX = 1;
            while (x + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (int16_t dy = 0; dy < extentY; ++dy) {
                    if (blockTypes[getIndex(x + extentX, y + dy, z)] != blockType ||
                        isFaceProcessed(processed[x + extentX][y + dy][z], FaceFlag::FRONT) ||
                        !isExposed(x + extentX, y + dy, z, 0, 0, 1, blockType)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (int16_t dy = 0; dy < extentY; ++dy) {
                    setFaceProcessed(processed[x + extentX][y + dy][z], FaceFlag::FRONT);
                }
                extentX++;
            }
            Block::addFrontFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;

        case 2: // Left face

            ao[0] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 0, 1, -1, blockType));
            ao[1] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 0, 1, 1, blockType));
            ao[2] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 0, -1, -1, blockType));
            ao[3] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 0, -1, 1, blockType));

            while (y + extentY < CHUNK_HEIGHT && blockTypes[getIndex(x, y + extentY, z)] == blockType &&
                !isFaceProcessed(processed[x][y + extentY][z], FaceFlag::LEFT) &&
                isExposed(x, y + extentY, z, -1, 0, 0, blockType)) {
                setFaceProcessed(processed[x][y + extentY][z], FaceFlag::LEFT);
                extentY++;
            }

            extentX = 1;
            while (z + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (int16_t dy = 0; dy < extentY; ++dy) {
                    if (blockTypes[getIndex(x, y + dy, z + extentX)] != blockType ||
                        isFaceProcessed(processed[x][y + dy][z + extentX], FaceFlag::LEFT) ||
                        !isExposed(x, y + dy, z + extentX, -1, 0, 0, blockType)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (int16_t dy = 0; dy < extentY; ++dy) {
                    setFaceProcessed(processed[x][y + dy][z + extentX], FaceFlag::LEFT);
                }
                extentX++;
            }
            Block::addLeftFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;

        case 3: // Right face

            ao[0] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 0, 1, -1, blockType));
            ao[1] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 0, 1, 1, blockType));
            ao[2] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 0, -1, -1, blockType));
            ao[3] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 0, -1, 1, blockType));

            while (y + extentY < CHUNK_HEIGHT && blockTypes[getIndex(x, y + extentY, z)] == blockType &&
                !isFaceProcessed(processed[x][y + extentY][z], FaceFlag::RIGHT) &&
                isExposed(x, y + extentY, z, 1, 0, 0, blockType)) {
                setFaceProcessed(processed[x][y + extentY][z], FaceFlag::RIGHT);
                extentY++;
            }

            extentX = 1;
            while (z + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (int16_t dy = 0; dy < extentY; ++dy) {
                    if (blockTypes[getIndex(x, y + dy, z + extentX)] != blockType ||
                        isFaceProcessed(processed[x][y + dy][z + extentX], FaceFlag::RIGHT) ||
                        !isExposed(x, y + dy, z + extentX, 1, 0, 0, blockType)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (int16_t dy = 0; dy < extentY; ++dy) {
                    setFaceProcessed(processed[x][y + dy][z + extentX], FaceFlag::RIGHT);
                }
                extentX++;
            }
            Block::addRightFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;

        case 4: // Top face

            while (z + extentY < CHUNK_SIZE && blockTypes[getIndex(x, y, z + extentY)] == blockType &&
                !isFaceProcessed(processed[x][y][z + extentY], FaceFlag::TOP) &&
                isExposed(x, y, z + extentY, 0, 1, 0, blockType)) {
                setFaceProcessed(processed[x][y][z + extentY], FaceFlag::TOP);
                extentY++;
            }

            extentX = 1;
            while (x + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (int16_t dz = 0; dz < extentY; ++dz) {
                    if (blockTypes[getIndex(x + extentX, y, z + dz)] != blockType ||
                        isFaceProcessed(processed[x + extentX][y][z + dz], FaceFlag::TOP) ||
                        !isExposed(x + extentX, y, z + dz, 0, 1, 0, blockType)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (int16_t dz = 0; dz < extentY; ++dz) {
                    setFaceProcessed(processed[x + extentX][y][z + dz], FaceFlag::TOP);
                }
                extentX++;
            }
            Block::addTopFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;

        case 5: // Bottom face

            ao[0] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, -1, 0, -1, blockType));
            ao[1] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 1, 0, -1, blockType));
            ao[2] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, -1, 0, 1, blockType));
            ao[3] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 1, 0, 1, blockType));

            while (z + extentY < CHUNK_SIZE && blockTypes[getIndex(x, y, z + extentY)] == blockType &&
                !isFaceProcessed(processed[x][y][z + extentY], FaceFlag::BOTTOM) &&
                isExposed(x, y, z + extentY, 0, -1, 0, blockType)) {
                setFaceProcessed(processed[x][y][z + extentY], FaceFlag::BOTTOM);
                extentY++;
            }
            extentX = 1;
            while (x + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (int16_t dz = 0; dz < extentY; ++dz) {
                    if (blockTypes[getIndex(x + extentX, y, z + dz)] != blockType ||
                        isFaceProcessed(processed[x + extentX][y][z + dz], FaceFlag::BOTTOM) ||
                        !isExposed(x + extentX, y, z + dz, 0, -1, 0, blockType)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (int16_t dz = 0; dz < extentY; ++dz) {
                    setFaceProcessed(processed[x + extentX][y][z + dz], FaceFlag::BOTTOM);
                }
                extentX++;
            }
            Block::addBottomFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;
        }
        };

    for (int16_t x = 0; x < CHUNK_SIZE; ++x) {
        for (int16_t y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int16_t z = 0; z < CHUNK_SIZE; ++z) {
                GLint index = getIndex(x, y, z);
                GLint blockType = blockTypes[index];
                if (blockTypes[index] == -1) continue;

                if (blockTypes[index] == 10 || blockTypes[index] == 11 || blockTypes[index] == 12 || blockTypes[index] == 13) // Flower 1 10, Grass 1 11, Grass 2 12, Grass 3 13
                {
                    // Add grass plant mesh
                    addGrassPlant(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, lightLevels[index], blockTypes[index]);
                    continue;
                }

                // Water block
                if (blockType == 4) {
                    uint8_t lightLevel = lightLevels[index];
                    GLint textureLayer = getTextureLayer(blockType, 0);
                    GLfloat ao[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

                    int16_t worldX = chunkX * CHUNK_SIZE + x;
                    int16_t worldZ = chunkZ * CHUNK_SIZE + z;

                    if (isExposed(x, y, z, 0, 0, -1, blockType)) Block::addBackFace(waterVertices, waterIndices, waterVertexOffset, worldX, y, worldZ, 1, 1, textureLayer, lightLevel, ao);
                    if (isExposed(x, y, z, 0, 0, 1, blockType)) Block::addFrontFace(waterVertices, waterIndices, waterVertexOffset, worldX, y, worldZ, 1, 1, textureLayer, lightLevel, ao);
                    if (isExposed(x, y, z, -1, 0, 0, blockType)) Block::addLeftFace(waterVertices, waterIndices, waterVertexOffset, worldX, y, worldZ, 1, 1, textureLayer, lightLevel, ao);
                    if (isExposed(x, y, z, 1, 0, 0, blockType)) Block::addRightFace(waterVertices, waterIndices, waterVertexOffset, worldX, y, worldZ, 1, 1, textureLayer, lightLevel, ao);
                    if (isExposed(x, y, z, 0, 1, 0, blockType)) 
                    {
                        GLfloat waterY = y + 0.9f;
                        Block::addTopFace(waterVertices, waterIndices, waterVertexOffset, worldX, waterY, worldZ, 1, 1, textureLayer, lightLevel, ao);
                    }
                    if (isExposed(x, y, z, 0, -1, 0, blockType)) Block::addBottomFace(waterVertices, waterIndices, waterVertexOffset, worldX, y, worldZ, 1, 1, textureLayer, lightLevel, ao);
                    continue;
                }

                // Back face
                if (!isFaceProcessed(processed[x][y][z], FaceFlag::BACK) && isExposed(x, y, z, 0, 0, -1, blockType)) {
                    processFace(x, y, z, 0);
                    setFaceProcessed(processed[x][y][z], FaceFlag::BACK);
                }
                // Front face
                if (!isFaceProcessed(processed[x][y][z], FaceFlag::FRONT) && isExposed(x, y, z, 0, 0, 1, blockType)) {
                    processFace(x, y, z, 1);
                    setFaceProcessed(processed[x][y][z], FaceFlag::FRONT);
                }
                // Left face
                if (!isFaceProcessed(processed[x][y][z], FaceFlag::LEFT) && isExposed(x, y, z, -1, 0, 0, blockType)) {
                    processFace(x, y, z, 2);
                    setFaceProcessed(processed[x][y][z], FaceFlag::LEFT);
                }
                // Right face
                if (!isFaceProcessed(processed[x][y][z], FaceFlag::RIGHT) && isExposed(x, y, z, 1, 0, 0, blockType)) {
                    processFace(x, y, z, 3);
                    setFaceProcessed(processed[x][y][z], FaceFlag::RIGHT);
                }
                // Top face
                if (!isFaceProcessed(processed[x][y][z], FaceFlag::TOP) && isExposed(x, y, z, 0, 1, 0, blockType)) {
                    processFace(x, y, z, 4);
                    setFaceProcessed(processed[x][y][z], FaceFlag::TOP);
                }
                // Bottom face
                if (!isFaceProcessed(processed[x][y][z], FaceFlag::BOTTOM) && y > 0 && isExposed(x, y, z, 0, -1, 0, blockType)) {
                    processFace(x, y, z, 5);
                    setFaceProcessed(processed[x][y][z], FaceFlag::BOTTOM);
                }
            }
        }
    }
}

GLint Chunk::getTextureLayer(int8_t blockType, int8_t face)
{
    switch (blockType) {
    case 0: return 0; // Dirt
    case 1: return 1; // Stone
    case 2: // Grass block
        if (face == 4) return 2; // Top face - Grass top
        else if (face == 5) return 0; // Bottom face - Dirt
        else return 3; // Side faces - Grass side
    case 3: return 4; // Sand
    case 4: return 5; // Water
    case 5: // Oak log
        if (face == 4 || face == 5) return 6; // Top and bottom faces
        else return 7; // Side faces
    case 6: return 8; // Oak leaf
    case 7: return 9; // Gravel
    case 8: return 10; // Cobblestone
    case 9: return 11; // Glass

    case 10: return 12; // Flower 1
    case 11: return 13; // Grass plant 1
    case 12: return 14; // Grass plant 2
    case 13: return 15; // Grass plant 3
    
    default: return 0; // Default to dirt
    }
}

void Chunk::addGrassPlant(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, GLint x, GLint y, GLint z, uint8_t lightLevel, GLint blockType)
{
    GLfloat size = 0.5f;
    GLfloat centerX = x + 0.5f;
    GLfloat centerZ = z + 0.5f;

    GLfloat positions[2][4][3] = {
        {   // First quad
            { centerX - size, y, centerZ - size },
            { centerX + size, y, centerZ + size },
            { centerX + size, y + 1.0f, centerZ + size },
            { centerX - size, y + 1.0f, centerZ - size }
        },
        {   // Second quad
            { centerX + size, y, centerZ - size },
            { centerX - size, y, centerZ + size },
            { centerX - size, y + 1.0f, centerZ + size },
            { centerX + size, y + 1.0f, centerZ - size }
        }
    };

    GLfloat texCoords[4][2] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f }
    };

    GLint textureLayer = getTextureLayer(blockType, 0);
    GLfloat normal[3] = { 0.0f, 1.0f, 0.0f };
    GLfloat grassLightLevel = 1.0f;
    GLfloat grassAO = 1.0f;

    vertices.reserve(vertices.size() + 8 * 10);
    indices.reserve(indices.size() + 6 * 2);

    for (int8_t quad = 0; quad < 2; ++quad)
    {
        for (int8_t i = 0; i < 4; ++i)
        {
            vertices.insert(vertices.end(), {
                positions[quad][i][0], positions[quad][i][1], positions[quad][i][2],   // Position
                texCoords[i][0], texCoords[i][1],                                      // Texture coordinates
                (GLfloat)textureLayer,                                                 // Texture layer
                normal[0], normal[1], normal[2],                                       // Normal
                grassLightLevel,                                                       // Light level
                grassAO                                                                // Ambient occlusion
            });
        }

        indices.insert(indices.end(), {
            static_cast<GLuint>(vertexOffset), static_cast<GLuint>(vertexOffset) + 1, static_cast<GLuint>(vertexOffset) + 2,
            static_cast<GLuint>(vertexOffset), static_cast<GLuint>(vertexOffset) + 2, static_cast<GLuint>(vertexOffset) + 3
        });
        vertexOffset += 4;
    }
}

GLint Chunk::getBlockType(GLint x, GLint y, GLint z) const {
    if ((unsigned)x >= CHUNK_SIZE || (unsigned)y >= CHUNK_HEIGHT || (unsigned)z >= CHUNK_SIZE) {
        return -1;
    }

    GLint index = x * CHUNK_HEIGHT * CHUNK_SIZE + y * CHUNK_SIZE + z;
    return blockTypes[index];
}

const std::vector<GLint>& Chunk::getBlockTypes() const
{
    return blockTypes;
}

void Chunk::setBlockType(GLint x, GLint y, GLint z, int8_t type)
{
    if ((unsigned)x >= CHUNK_SIZE || (unsigned)y >= CHUNK_HEIGHT || (unsigned)z >= CHUNK_SIZE) {
        return;
    }

    GLint index = x * CHUNK_HEIGHT * CHUNK_SIZE + y * CHUNK_SIZE + z;
    if (blockTypes[index] != type) {
        blockTypes[index] = type;
        needsMeshUpdate = true;
    }
}

void Chunk::Draw()
{
    if (indices.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glDisable(GL_BLEND);
}

void Chunk::DrawWater(shader& waterShader, glm::mat4 view, glm::mat4 projection, glm::vec3 lightDirection, Camera& camera)
{
    if (waterIndices.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat timeValue = glfwGetTime();
    GLfloat waveSpeed = 2.3f;
    GLfloat waveFrequency = 1.2f;
    GLfloat waveAmplitude = 0.05f;

    std::vector<GLfloat> modifiedWaterVertices = waterVertices;

    for (size_t i = 0; i < modifiedWaterVertices.size(); i += 11)
    {
        GLfloat x = modifiedWaterVertices[i];
        GLfloat z = modifiedWaterVertices[i + 2];

        modifiedWaterVertices[i + 1] += sin(x * waveFrequency + timeValue * waveSpeed) * waveAmplitude +
                                        cos(z * waveFrequency + timeValue * waveSpeed) * waveAmplitude;
    }

    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, modifiedWaterVertices.size() * sizeof(GLfloat), modifiedWaterVertices.data());

    waterShader.use();
    waterShader.setMat4("model", glm::mat4(1.0f));
    waterShader.setMat4("view", view);
    waterShader.setMat4("projection", projection);
    waterShader.setVec3("lightDirection", lightDirection);
    waterShader.setVec3("viewPos", camera.getPosition());

    glBindVertexArray(waterVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
    glDrawElements(GL_TRIANGLES, waterIndices.size(), GL_UNSIGNED_INT, 0);

    glDisable(GL_BLEND);

    glBindVertexArray(0);
}

void Chunk::updateOpenGLBuffers()
{
    if (VAO == 0)
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Texture layer attribute
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    
    // Normal attribute
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    // Light level attribute
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(9 * sizeof(GLfloat)));
    glEnableVertexAttribArray(4);

    // Ambient occlusion attribute
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(10 * sizeof(GLfloat)));
    glEnableVertexAttribArray(5);

    glBindVertexArray(0);
}

void Chunk::updateOpenGLWaterBuffers()
{
    if (waterVAO == 0)
    {
        glGenVertexArrays(1, &waterVAO);
        glGenBuffers(1, &waterVBO);
        glGenBuffers(1, &waterEBO);
    }

    glBindVertexArray(waterVAO);

    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, waterVertices.size() * sizeof(GLfloat), waterVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterIndices.size() * sizeof(GLuint), waterIndices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Texture layer attribute
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    // Normal attribute
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    // Light level attribute
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(9 * sizeof(GLfloat)));
    glEnableVertexAttribArray(4);

    // Ambient occlusion attribute
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(10 * sizeof(GLfloat)));
    glEnableVertexAttribArray(5);

    glBindVertexArray(0);
}

void Chunk::calculateBounds() {
    minBounds = glm::vec3(chunkX * CHUNK_SIZE, 0, chunkZ * CHUNK_SIZE);
    maxBounds = glm::vec3((chunkX + 1) * CHUNK_SIZE, CHUNK_HEIGHT, (chunkZ + 1) * CHUNK_SIZE);
}

bool Chunk::isInFrustum(const Frustum& frustum) const {
    if (!isInitialized) return false;
    return frustum.isBoxInFrustum(minBounds, maxBounds);
}