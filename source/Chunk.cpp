#include "Chunk.h"
#include "World.h"

Chunk::Chunk(GLint x, GLint z, TextureManager& textureManager, World* world)
    : chunkX(x), chunkZ(z), textureManager(textureManager), textureID(textureManager.getTextureID()), world(world), 
      forestBiome(BiomeTypes::Forest), desertBiome(BiomeTypes::Desert), plainsBiome(BiomeTypes::Plains), mountainBiome(BiomeTypes::Mountain)
{
    minBounds = glm::vec3(chunkX * CHUNK_SIZE, 0, chunkZ * CHUNK_SIZE);
    maxBounds = glm::vec3((chunkX + 1) * CHUNK_SIZE, CHUNK_HEIGHT, (chunkZ + 1) * CHUNK_SIZE);

    determineBiomeType(x, z);
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
    generateChunk();
    generateMesh(blockTypes);
}

Biomes Chunk::determineBiomeType(GLint x, GLint z)
{
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    biomeNoise.SetFrequency(0.0003f);

    caveNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    caveNoise.SetFractalOctaves(5);
    caveNoise.SetFrequency(0.016f);

    GLfloat noiseValue = biomeNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z));

    if (noiseValue < -0.3f)
    {
        return Biomes(BiomeTypes::Desert);
    }
    else if (noiseValue < 0.3f)
    {
        return Biomes(BiomeTypes::Plains);
    }
    else
    {
        return Biomes(BiomeTypes::Forest);
    }
}

const BiomeData* Chunk::selectBiome(GLfloat noiseValue) {
    for (const auto& biome : biomes) {
        if (noiseValue >= biome.minThreshold && noiseValue < biome.maxThreshold) {
            return &biome;
        }
    }
    return nullptr;
}

const Biomes* Chunk::getBiomeInstance(BiomeTypes type) const {
    switch (type) {
    case BiomeTypes::Forest:
        return &forestBiome;
    case BiomeTypes::Desert:
        return &desertBiome;
    case BiomeTypes::Plains:
        return &plainsBiome;
    
    default:
        return nullptr;
    }
}

GLfloat Chunk::smoothstep(GLfloat edge0, GLfloat edge1, GLfloat x) {
    x = (x - edge0) / (edge1 - edge0);
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3 - 2 * x);
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

            GLfloat biomeNoiseValue = biomeNoise.GetNoise(static_cast<GLfloat>(globalX), static_cast<GLfloat>(globalZ));
            GLfloat biomeValue = (biomeNoiseValue + 1.0f) / 2.0f;

            std::vector<GLfloat> biomeWeights(biomes.size(), 0.0f);

            for (size_t i = 0; i < biomes.size(); ++i) {
                const BiomeData& biome = biomes[i];
                GLfloat weight = 0.0f;

                if (biome.type == BiomeTypes::Desert) {
                    weight = 1.0f - smoothstep(biome.edge0, biome.edge1, biomeValue);
                }
                else if (biome.type == BiomeTypes::Plains) {
                    weight = smoothstep(biome.edge0, biome.edge1, biomeValue) * (1.0f - smoothstep(biome.edge2, biome.edge3, biomeValue));
                }
                else if (biome.type == BiomeTypes::Forest) {
                    weight = smoothstep(biome.edge0, biome.edge1, biomeValue);
                }
                else if (biome.type == BiomeTypes::Mountain) {
                    weight = smoothstep(biome.edge0, biome.edge1, biomeValue) * (1.0f - smoothstep(biome.edge2, biome.edge3, biomeValue));
                }
                biomeWeights[i] = weight;
            }

            GLfloat totalWeight = std::accumulate(biomeWeights.begin(), biomeWeights.end(), 0.0f);
            if (totalWeight > 0.0f) {
                for (GLfloat& weight : biomeWeights) {
                    weight /= totalWeight;
                }
            }

            // Blend terrain heights
            GLfloat blendedHeight = 0.0f;
            for (size_t i = 0; i < biomes.size(); ++i) {
                if (biomeWeights[i] > 0.0f) {
                    const BiomeData& biome = biomes[i];
                    Biomes* biomeInstance = biomeInstances[biome.type];
                    GLfloat height = biomeInstance->getTerrainHeightAt(globalX, globalZ);
                    blendedHeight += height * biomeWeights[i];
                }
            }
            uint16_t terrainHeight = static_cast<uint16_t>(blendedHeight);

            size_t maxWeightBiomeIndex = std::distance(biomeWeights.begin(), std::max_element(biomeWeights.begin(), biomeWeights.end()));
            const BiomeData& primaryBiome = biomes[maxWeightBiomeIndex];
            Biomes* biomeInstance = biomeInstances[primaryBiome.type];

            GLfloat biomeCaveThreshold = 0.0f;
            for (size_t i = 0; i < biomes.size(); ++i) {
                biomeCaveThreshold += biomeWeights[i] * biomes[i].caveThreshold;
            }

            for (uint8_t y = 0; y < CHUNK_HEIGHT; ++y)
            {
                uint16_t index = getIndex(x, y, z);

                GLfloat caveValue = caveNoise.GetNoise(static_cast<GLfloat>(globalX), static_cast<GLfloat>(y), static_cast<GLfloat>(globalZ));
                bool isCave = (caveValue > biomeCaveThreshold) && y > caveMinHeight && y < (CHUNK_HEIGHT - surfaceBuffer);

                if (isCave)
                {
                    blockTypes[index] = -1;
                    continue;
                }

                if (y > terrainHeight)
                {
                    if (y <= WATERLEVEL)
                    {
                        blockTypes[index] = WATER;
                    }
                    continue;
                }

                if (y <= WATERLEVEL && y >= terrainHeight - 2)
                {
                    blockTypes[index] = SAND;
                }
                else if (y == terrainHeight) {
                    blockTypes[index] = biomeInstance->getSurfaceBlock(y);
                }
                else if (y >= terrainHeight - 4) {
                    blockTypes[index] = biomeInstance->getSubSurfaceBlock();
                }
                else {
                    blockTypes[index] = biomeInstance->getUndergroundBlock();
                }
            }

            uint16_t indexBelow = getIndex(x, terrainHeight, z);
            uint16_t indexAbove = getIndex(x, terrainHeight + 1, z);

            if (world->isStructureGenerationEnabled) {
                if (blockTypes[indexBelow] != -1 && blockTypes[indexAbove] == -1)
                {
                    GLfloat randomValue = static_cast<GLfloat>(rand()) / RAND_MAX;

                    GLfloat weightThreshold = 0.0f;
                    for (size_t i = 0; i < biomes.size(); ++i) {
                        weightThreshold += biomeWeights[i];
                        if (randomValue <= weightThreshold) {
                            const BiomeData& biome = biomes[i];
                            Biomes* biomeInstance = biomeInstances[biome.type];

                            // -- FOREST -- //
                            if (biome.type == BiomeTypes::Forest && blockTypes[indexBelow] == GRASS_BLOCK)
                            {

                                if (biomeInstance->shouldPlaceTree(globalX, globalZ))
                                {
                                    uint8_t randomTreeType = rand() % 3;
                                    if (randomTreeType == 0)
                                        Structure::generateBaseProceduralTree(*this, x, terrainHeight + 1, z);
                                    else if (randomTreeType == 1)
                                        Structure::generateProceduralTreeOrangeLeaves(*this, x, terrainHeight + 1, z);
                                    else
                                        Structure::generateProceduralTreeYellowLeaves(*this, x, terrainHeight + 1, z);
                                }
                                else if (biomeInstance->shouldPlaceGrass(globalX, globalZ))
                                {
                                    uint8_t grassType = forestBiome.getRandomGrassType();
                                    if (grassType != -1)
                                    {
                                        blockTypes[indexAbove] = grassType;
                                    }
                                }
                                else if (biomeInstance->shouldPlaceFlower(globalX, globalZ))
                                {
                                    uint8_t flowerType = forestBiome.getRandomFlowerType();
                                    if (flowerType != -1)
                                    {
                                        blockTypes[indexAbove] = flowerType;
                                    }
                                }
                            }
                            // -- PLAINS -- //
                            else if (biome.type == BiomeTypes::Plains && blockTypes[indexBelow] == GRASS_BLOCK)
                            {
                                if (plainsBiome.shouldPlaceTree(globalX, globalZ))
                                {
                                    Structure::generateBasePurpleTree(*this, x, terrainHeight + 1, z);
                                }
                                else if (plainsBiome.shouldPlaceGrass(globalX, globalZ)) {
                                    blockTypes[indexAbove] = plainsBiome.getRandomGrassType();
                                }
                                else if (plainsBiome.shouldPlaceFlower(globalX, globalZ)) {
                                    blockTypes[indexAbove] = plainsBiome.getRandomFlowerType();
                                }
                            }
                            // -- DESERT -- //
                            else if (biome.type == BiomeTypes::Desert && blockTypes[indexBelow] == SAND)
                            {
                                if (biomeInstance->shouldPlaceDeadBush(globalX, globalZ))
                                {
                                    blockTypes[indexAbove] = DEADBUSH;
                                }
                            }

                            break;
                        }
                    }
                }
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

GLint Chunk::getTerrainHeightAt(GLint x, GLint z)
{
    GLfloat biomeNoiseValue = biomeNoise.GetNoise(static_cast<GLfloat>(x), static_cast<GLfloat>(z));
    GLfloat biomeValue = (biomeNoiseValue + 1.0f) / 2.0f;

    std::vector<GLfloat> biomeWeights(biomes.size(), 0.0f);

    for (size_t i = 0; i < biomes.size(); ++i) {
        const BiomeData& biome = biomes[i];
        GLfloat weight = 0.0f;

        if (biome.type == BiomeTypes::Desert) {
            weight = 1.0f - smoothstep(biome.edge0, biome.edge1, biomeValue);
        }
        else if (biome.type == BiomeTypes::Plains) {
            weight = smoothstep(biome.edge0, biome.edge1, biomeValue) * (1.0f - smoothstep(biome.edge2, biome.edge3, biomeValue));
        }
        else if (biome.type == BiomeTypes::Forest) {
            weight = smoothstep(biome.edge0, biome.edge1, biomeValue);
        }
        else if (biome.type == BiomeTypes::Mountain) {
            weight = smoothstep(biome.edge0, biome.edge1, biomeValue) * (1.0f - smoothstep(biome.edge2, biome.edge3, biomeValue));
        }

        biomeWeights[i] = weight;
    }

    GLfloat totalWeight = std::accumulate(biomeWeights.begin(), biomeWeights.end(), 0.0f);
    if (totalWeight > 0.0f) {
        for (GLfloat& weight : biomeWeights) {
            weight /= totalWeight;
        }
    }

    GLfloat blendedHeight = 0.0f;
    for (size_t i = 0; i < biomes.size(); ++i) {
        if (biomeWeights[i] > 0.0f) {
            const BiomeData& biome = biomes[i];
            const Biomes* biomeInstance = getBiomeInstance(biome.type);
            GLfloat biomeHeight = biomeInstance->getTerrainHeightAt(x, z);
            blendedHeight += biomeHeight * biomeWeights[i];
        }
    }

    return static_cast<int16_t>(blendedHeight);
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
    return blockType == -1 || blockType == WATER || blockType == OAK_LEAF || blockType == OAK_LEAF_ORANGE || blockType == OAK_LEAF_YELLOW || blockType == GLASS ||
        blockType == FLOWER1 || blockType == FLOWER2 || blockType == FLOWER3 || blockType == FLOWER4 || blockType == FLOWER5 || blockType == GRASS1 || blockType == GRASS2 || 
        blockType == GRASS3 || blockType == DEADBUSH || blockType == OAK_LEAF_PURPLE || blockType == TORCH;
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

            if (world->getAOState())
            {
                ao[0] = calculateAO(isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, -1, 1, 0, blockType));
                ao[1] = calculateAO(isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 1, 1, 0, blockType));
                ao[2] = calculateAO(isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, -1, -1, 0, blockType));
                ao[3] = calculateAO(isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 1, -1, 0, blockType));
            }
            
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

            if (world->getAOState())
            {
                ao[0] = calculateAO(isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, -1, 1, 1, blockType));
                ao[1] = calculateAO(isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 1, 1, 1, blockType));
                ao[2] = calculateAO(isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, -1, -1, 1, blockType));
                ao[3] = calculateAO(isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 1, -1, 1, blockType));
            }

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

            if (world->getAOState())
            {
                ao[0] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 0, 1, -1, blockType));
                ao[1] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 0, 1, 1, blockType));
                ao[2] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 0, -1, -1, blockType));
                ao[3] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 0, -1, 1, blockType));
            }

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

            if (world->getAOState())
            {
                ao[0] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 0, 1, -1, blockType));
                ao[1] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, 0, 1, 0, blockType), isExposed(x, y, z, 0, 1, 1, blockType));
                ao[2] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 0, -1, -1, blockType));
                ao[3] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, 0, -1, 0, blockType), isExposed(x, y, z, 0, -1, 1, blockType));
            }

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

            if (world->getAOState())
            {
                ao[0] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, -1, 0, -1, blockType));
                ao[1] = calculateAO(isExposed(x, y, z, 0, 0, -1, blockType), isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 1, 0, -1, blockType));
                ao[2] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, -1, 0, 0, blockType), isExposed(x, y, z, -1, 0, 1, blockType));
                ao[3] = calculateAO(isExposed(x, y, z, 0, 0, 1, blockType), isExposed(x, y, z, 1, 0, 0, blockType), isExposed(x, y, z, 1, 0, 1, blockType));
            }

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

                if (blockTypes[index] == FLOWER1 || blockType == FLOWER2 || blockType == FLOWER3 || blockType == FLOWER4 || blockType == FLOWER5 
                    || blockTypes[index] == GRASS1 || blockTypes[index] == GRASS2 || blockTypes[index] == GRASS3 || blockTypes[index] == DEADBUSH)
                {
                    // Add grass plant mesh
                    addGrassPlant(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, lightLevels[index], blockTypes[index]);
                    continue;
                }

                if (blockTypes[index] == TORCH)
                {
                    // Add torch mesh
                    addTorch(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, lightLevels[index], blockTypes[index]);
                    continue;
                }

                // Water block
                if (blockType == WATER) {
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
    case DIRT: return 0;
    case STONE: return 1;

    case GRASS_BLOCK:
        if (face == TOP) return 2;
        else if (face == BOTTOM) return 0;
        else return 3;

    case SAND: return 4;
    case WATER: return 5;

    case OAK_LOG:
        if (face == TOP || face == BOTTOM) return 6;
        else return 7;

    case OAK_LEAF: return 8;
    case GRAVEL: return 9;
    case COBBLESTONE: return 10;
    case GLASS: return 11;

    case FLOWER1: return 12;
    case GRASS1: return 13;
    case GRASS2: return 14;
    case GRASS3: return 15;
    case FLOWER2: return 16;
    case FLOWER3: return 17;
    case FLOWER4: return 18;
    case FLOWER5: return 19;

    case OAK_LEAF_ORANGE: return 20;
    case OAK_LEAF_YELLOW: return 21;

    case DEADBUSH: return 22;

    case OAK_LEAF_PURPLE: return 23;

    case SNOW: return 24;

    case TORCH: 
        if (face == TOP || face == BOTTOM) return 26;
        else return 25;
    
    default: return DIRT;
    }
}

void Chunk::addTorch(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, GLint x, GLint y, GLint z, uint8_t lightLevel, GLint blockType)
{
    GLfloat torchHeight = 1.5f;

    GLfloat y0 = y;
    GLfloat y1 = y + torchHeight;

    GLfloat frontWidth = 1.2f;
    GLfloat backWidth = 1.2f;
    GLfloat leftDepth = 1.2f;
    GLfloat rightDepth = 1.2f;
    GLfloat topWidth = 0.145f;

    GLfloat xCenter = x + 0.5f;
    GLfloat zCenter = z + 0.5f;

    GLfloat frontX0 = xCenter - frontWidth / 2.0f;
    GLfloat frontX1 = xCenter + frontWidth / 2.0f;
    GLfloat frontZ = zCenter - 0.07f;

    GLfloat backX0 = xCenter - backWidth / 2.0f;
    GLfloat backX1 = xCenter + backWidth / 2.0f;
    GLfloat backZ = zCenter + 0.07f;

    GLfloat leftX = xCenter - 0.07f;
    GLfloat rightX = xCenter + 0.07f;
    GLfloat leftZ0 = zCenter - leftDepth / 2.0f;
    GLfloat leftZ1 = zCenter + leftDepth / 2.0f;
    GLfloat rightZ0 = zCenter - rightDepth / 2.0f;
    GLfloat rightZ1 = zCenter + rightDepth / 2.0f;

    GLfloat topX0 = xCenter - topWidth / 2.0f;
    GLfloat topX1 = xCenter + topWidth / 2.0f;
    GLfloat topZ0 = zCenter - topWidth / 2.0f;
    GLfloat topZ1 = zCenter + topWidth / 2.0f;

    GLfloat texCoords[4][2] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f }
    };

    GLfloat torchLightLevel = 1.0f;
    GLfloat torchAO = 1.0f;

    GLfloat normals[5][3] = {
        {  0.0f,  0.0f, -1.0f }, // Front face
        {  0.0f,  0.0f,  1.0f }, // Back face
        { -1.0f,  0.0f,  0.0f }, // Left face
        {  1.0f,  0.0f,  0.0f }, // Right face
        {  0.0f,  1.0f,  0.0f }  // Top face only
    };

    GLfloat positions[5][4][3] = {
        // Front face
        {
            { frontX0, y0, frontZ },
            { frontX1, y0, frontZ },
            { frontX1, y1, frontZ },
            { frontX0, y1, frontZ }
        },
        // Back face
        {
            { backX1, y0, backZ },
            { backX0, y0, backZ },
            { backX0, y1, backZ },
            { backX1, y1, backZ }
        },
        // Left face
        {
            { leftX, y0, leftZ1 },
            { leftX, y0, leftZ0 },
            { leftX, y1, leftZ0 },
            { leftX, y1, leftZ1 }
        },
        // Right face
        {
            { rightX, y0, rightZ0 },
            { rightX, y0, rightZ1 },
            { rightX, y1, rightZ1 },
            { rightX, y1, rightZ0 }
        },
        // Top face
        {
            { topX0, y1 - 0.566f, topZ0 },
            { topX1, y1 - 0.566f, topZ0 },
            { topX1, y1 - 0.566f, topZ1 },
            { topX0, y1 - 0.566f, topZ1 }
        }
    };

    int8_t faces[5] = { 0, 1, 2, 3, TOP };

    for (uint8_t face = 0; face < 5; ++face)
    {
        GLint textureLayer = getTextureLayer(blockType, faces[face]);

        for (uint8_t i = 0; i < 4; ++i)
        {
            vertices.insert(vertices.end(), {
                positions[face][i][0], positions[face][i][1], positions[face][i][2],  // Position
                texCoords[i][0], texCoords[i][1],                                     // Texture coordinates
                (GLfloat)textureLayer,                                                // Texture layer
                normals[face][0], normals[face][1], normals[face][2],                 // Normal
                torchLightLevel,                                                      // Light level
                torchAO                                                               // Ambient occlusion
            });
        }

        indices.insert(indices.end(), {
            static_cast<GLuint>(vertexOffset),     static_cast<GLuint>(vertexOffset) + 1, static_cast<GLuint>(vertexOffset) + 2,
            static_cast<GLuint>(vertexOffset),     static_cast<GLuint>(vertexOffset) + 2, static_cast<GLuint>(vertexOffset) + 3
        });

        vertexOffset += 4;
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