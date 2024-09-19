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
}

void Chunk::setupChunk()
{
    generateChunk();
    generateMesh(blockTypes);
}

void Chunk::generateChunk()
{
    FastNoiseLite baseNoise, elevationNoise, caveNoise;

    // Base noise
    baseNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    baseNoise.SetFrequency(0.005f);

    // Elevation noise
    elevationNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    elevationNoise.SetFrequency(0.001f);

    // Cave noise
    caveNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    caveNoise.SetFrequency(0.009f);
    caveNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    caveNoise.SetFractalOctaves(4);

    blockTypes.resize(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE, -1);

    uint8_t caveMinHeight = CHUNK_HEIGHT / 64;
    uint8_t surfaceBuffer = CHUNK_HEIGHT / 10;

    for (uint8_t x = 0; x < CHUNK_SIZE; ++x)
    {
        for (uint8_t z = 0; z < CHUNK_SIZE; ++z)
        {
            GLfloat worldX = static_cast<GLfloat>(chunkX * CHUNK_SIZE + x);
            GLfloat worldZ = static_cast<GLfloat>(chunkZ * CHUNK_SIZE + z);

            // Generate base terrain height
            GLfloat baseHeight = baseNoise.GetNoise(worldX, worldZ);
            GLfloat elevation = elevationNoise.GetNoise(worldX, worldZ);

            uint8_t terrainHeight = static_cast<uint8_t>((baseHeight + elevation * 0.5f + 1.0f) * 0.5f * (CHUNK_HEIGHT - 30)) + 30;

            for (uint8_t y = 0; y < CHUNK_HEIGHT; ++y)
            {
                GLuint index = x * CHUNK_HEIGHT * CHUNK_SIZE + y * CHUNK_SIZE + z;

                // Generate cave structures
                GLfloat caveValue = caveNoise.GetNoise(worldX, static_cast<GLfloat>(y), worldZ);
                bool isCave = caveValue > 0.37f && y > caveMinHeight && y < (CHUNK_HEIGHT - surfaceBuffer);

                if (isCave)
                {
                    blockTypes[index] = -1; // Air block for caves
                    continue;
                }

                if (y > terrainHeight)
                {
                    if (y <= WATERLEVEL)
                    {
                        blockTypes[index] = 4; // Water
                    }
                    else
                    {
                        blockTypes[index] = -1; // Air above the terrain
                    }
                    continue;
                }

                if (y >= WATERLEVEL - 2 && y <= terrainHeight && terrainHeight <= WATERLEVEL + 2)
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

    lightLevels.resize(blockTypes.size(), 0);

    // Calculate light levels
    for (uint8_t x = 0; x < CHUNK_SIZE; ++x)
    {
        for (uint8_t z = 0; z < CHUNK_SIZE; ++z)
        {
            recalculateSunlightColumn(x, z);
        }
    }
}

void Chunk::recalculateSunlightColumn(GLint x, GLint z) {
    uint8_t lightLevel = 15; // Maximum sunlight

    // Loop through the column from top to bottom
    for (GLint y = CHUNK_HEIGHT - 1; y >= 0; --y) {
        GLuint index = x * CHUNK_HEIGHT * CHUNK_SIZE + y * CHUNK_SIZE + z;

        if (blockTypes[index] == -1) { // Air block
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
    GLint vertexOffset = 0;

    struct ProcessedFaces {
        bool back = false;
        bool front = false;
        bool left = false;
        bool right = false;
        bool top = false;
        bool bottom = false;
    };

    std::vector<std::vector<std::vector<ProcessedFaces>>> processed(
        CHUNK_SIZE, std::vector<std::vector<ProcessedFaces>>(CHUNK_HEIGHT, std::vector<ProcessedFaces>(CHUNK_SIZE)));

    auto getIndex = [this](GLint x, GLint y, GLint z) {
        return x * CHUNK_HEIGHT * CHUNK_SIZE + y * CHUNK_SIZE + z;
    };

    auto isExposed = [&](GLint x, GLint y, GLint z, GLint dx, GLint dy, GLint dz) {
        GLint nx = x + dx, ny = y + dy, nz = z + dz;

        if (nx >= 0 && ny >= 0 && nz >= 0 && nx < CHUNK_SIZE && nz < CHUNK_SIZE && ny < CHUNK_HEIGHT) {
            return blockTypes[getIndex(nx, ny, nz)] == -1;
        }

        GLint neighborChunkX = chunkX, neighborChunkZ = chunkZ;

        if (nx < 0) neighborChunkX -= 1;
        else if (nx >= CHUNK_SIZE) neighborChunkX += 1;

        if (nz < 0) neighborChunkZ -= 1;
        else if (nz >= CHUNK_SIZE) neighborChunkZ += 1;

        if (Chunk* neighborChunk = world->getChunk(neighborChunkX, neighborChunkZ)) {
            nx = (nx + CHUNK_SIZE) % CHUNK_SIZE;
            nz = (nz + CHUNK_SIZE) % CHUNK_SIZE;
            return neighborChunk->getBlockType(nx, ny, nz) == -1;
        }

        return true;
    };

    auto getTextureLayer = [&](int8_t blockType, int8_t face) {
        switch (blockType) {
        case 0: return 0; // Dirt
        case 1: return 1; // Stone
        case 2: // Grass
            if (face == 4) return 2; // Top face - Grass top
            else if (face == 5) return 0; // Bottom face - Dirt
            else return 3; // Side faces - Grass side
        case 3: return 4; // Sand
        case 4: return 5; // Water
        default: return 0; // Default to dirt
        }
    };

    auto calculateAO = [&](bool side1, bool side2, bool corner) {
        GLfloat baseAO = 1.0f;
        if (side1 && side2) baseAO = 1.0f;
        else baseAO = 1.0f - (side1 + side2 + corner) / 3.0f;
        return glm::clamp(baseAO, 0.90f, 1.0f);
    };

    auto processFace = [&](GLint x, GLint y, GLint z, int8_t face) {
        GLint extentX = 1;
        GLint extentY = 1;
        GLint blockType = blockTypes[getIndex(x, y, z)];
        GLint textureLayer = getTextureLayer(blockType, face);
        GLuint index = getIndex(x, y, z);
        uint8_t lightLevel = lightLevels[index];
        GLfloat ao[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        switch (face) {
        case 0: // Back face

            ao[0] = calculateAO(isExposed(x, y, z, -1, 0, 0), isExposed(x, y, z, 0, 1, 0), isExposed(x, y, z, -1, 1, 0));
            ao[1] = calculateAO(isExposed(x, y, z, 1, 0, 0), isExposed(x, y, z, 0, 1, 0), isExposed(x, y, z, 1, 1, 0));
            ao[2] = calculateAO(isExposed(x, y, z, -1, 0, 0), isExposed(x, y, z, 0, -1, 0), isExposed(x, y, z, -1, -1, 0));
            ao[3] = calculateAO(isExposed(x, y, z, 1, 0, 0), isExposed(x, y, z, 0, -1, 0), isExposed(x, y, z, 1, -1, 0));

            while (y + extentY < CHUNK_HEIGHT && blockTypes[getIndex(x, y + extentY, z)] == blockType &&
                !processed[x][y + extentY][z].back && isExposed(x, y + extentY, z, 0, 0, -1)) {
                processed[x][y + extentY][z].back = true;
                extentY++;
            }
            extentX = 1;
            while (x + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (GLint dy = 0; dy < extentY; ++dy) {
                    if (blockTypes[getIndex(x + extentX, y + dy, z)] != blockType ||
                        processed[x + extentX][y + dy][z].back ||
                        !isExposed(x + extentX, y + dy, z, 0, 0, -1)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (GLint dy = 0; dy < extentY; ++dy) {
                    processed[x + extentX][y + dy][z].back = true;
                }
                extentX++;
            }
            Block::addBackFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;

        case 1: // Front face

            ao[0] = calculateAO(isExposed(x, y, z, -1, 0, 0), isExposed(x, y, z, 0, 1, 0), isExposed(x, y, z, -1, 1, 1));
            ao[1] = calculateAO(isExposed(x, y, z, 1, 0, 0), isExposed(x, y, z, 0, 1, 0), isExposed(x, y, z, 1, 1, 1));
            ao[2] = calculateAO(isExposed(x, y, z, -1, 0, 0), isExposed(x, y, z, 0, -1, 0), isExposed(x, y, z, -1, -1, 1));
            ao[3] = calculateAO(isExposed(x, y, z, 1, 0, 0), isExposed(x, y, z, 0, -1, 0), isExposed(x, y, z, 1, -1, 1));

            while (y + extentY < CHUNK_HEIGHT && blockTypes[getIndex(x, y + extentY, z)] == blockType &&
                !processed[x][y + extentY][z].front && isExposed(x, y + extentY, z, 0, 0, 1)) {
                processed[x][y + extentY][z].front = true;
                extentY++;
            }
            extentX = 1;
            while (x + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (GLint dy = 0; dy < extentY; ++dy) {
                    if (blockTypes[getIndex(x + extentX, y + dy, z)] != blockType ||
                        processed[x + extentX][y + dy][z].front ||
                        !isExposed(x + extentX, y + dy, z, 0, 0, 1)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (GLint dy = 0; dy < extentY; ++dy) {
                    processed[x + extentX][y + dy][z].front = true;
                }
                extentX++;
            }
            Block::addFrontFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;

        case 2: // Left face

            ao[0] = calculateAO(isExposed(x, y, z, 0, 0, -1), isExposed(x, y, z, 0, 1, 0), isExposed(x, y, z, 0, 1, -1));
            ao[1] = calculateAO(isExposed(x, y, z, 0, 0, 1), isExposed(x, y, z, 0, 1, 0), isExposed(x, y, z, 0, 1, 1));
            ao[2] = calculateAO(isExposed(x, y, z, 0, 0, -1), isExposed(x, y, z, 0, -1, 0), isExposed(x, y, z, 0, -1, -1));
            ao[3] = calculateAO(isExposed(x, y, z, 0, 0, 1), isExposed(x, y, z, 0, -1, 0), isExposed(x, y, z, 0, -1, 1));

            while (y + extentY < CHUNK_HEIGHT && blockTypes[getIndex(x, y + extentY, z)] == blockType &&
                !processed[x][y + extentY][z].left && isExposed(x, y + extentY, z, -1, 0, 0)) {
                processed[x][y + extentY][z].left = true;
                extentY++;
            }
            extentX = 1;
            while (z + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (GLint dy = 0; dy < extentY; ++dy) {
                    if (blockTypes[getIndex(x, y + dy, z + extentX)] != blockType ||
                        processed[x][y + dy][z + extentX].left ||
                        !isExposed(x, y + dy, z + extentX, -1, 0, 0)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (GLint dy = 0; dy < extentY; ++dy) {
                    processed[x][y + dy][z + extentX].left = true;
                }
                extentX++;
            }
            Block::addLeftFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;

        case 3: // Right face

            ao[0] = calculateAO(isExposed(x, y, z, 0, 0, -1), isExposed(x, y, z, 0, 1, 0), isExposed(x, y, z, 0, 1, -1));
            ao[1] = calculateAO(isExposed(x, y, z, 0, 0, 1), isExposed(x, y, z, 0, 1, 0), isExposed(x, y, z, 0, 1, 1));
            ao[2] = calculateAO(isExposed(x, y, z, 0, 0, -1), isExposed(x, y, z, 0, -1, 0), isExposed(x, y, z, 0, -1, -1));
            ao[3] = calculateAO(isExposed(x, y, z, 0, 0, 1), isExposed(x, y, z, 0, -1, 0), isExposed(x, y, z, 0, -1, 1));

            while (y + extentY < CHUNK_HEIGHT && blockTypes[getIndex(x, y + extentY, z)] == blockType &&
                !processed[x][y + extentY][z].right && isExposed(x, y + extentY, z, 1, 0, 0)) {
                processed[x][y + extentY][z].right = true;
                extentY++;
            }
            extentX = 1;
            while (z + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (GLint dy = 0; dy < extentY; ++dy) {
                    if (blockTypes[getIndex(x, y + dy, z + extentX)] != blockType ||
                        processed[x][y + dy][z + extentX].right ||
                        !isExposed(x, y + dy, z + extentX, 1, 0, 0)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (GLint dy = 0; dy < extentY; ++dy) {
                    processed[x][y + dy][z + extentX].right = true;
                }
                extentX++;
            }
            Block::addRightFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;

        case 4: // Top face

            while (z + extentY < CHUNK_SIZE && blockTypes[getIndex(x, y, z + extentY)] == blockType &&
                !processed[x][y][z + extentY].top && isExposed(x, y, z + extentY, 0, 1, 0)) {
                processed[x][y][z + extentY].top = true;
                extentY++;
            }
            extentX = 1;
            while (x + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (GLint dz = 0; dz < extentY; ++dz) {
                    if (blockTypes[getIndex(x + extentX, y, z + dz)] != blockType ||
                        processed[x + extentX][y][z + dz].top ||
                        !isExposed(x + extentX, y, z + dz, 0, 1, 0)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (GLint dz = 0; dz < extentY; ++dz) {
                    processed[x + extentX][y][z + dz].top = true;
                }
                extentX++;
            }
            Block::addTopFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;

        case 5: // Bottom face

            ao[0] = calculateAO(isExposed(x, y, z, 0, 0, -1), isExposed(x, y, z, -1, 0, 0), isExposed(x, y, z, -1, 0, -1));
            ao[1] = calculateAO(isExposed(x, y, z, 0, 0, -1), isExposed(x, y, z, 1, 0, 0), isExposed(x, y, z, 1, 0, -1));
            ao[2] = calculateAO(isExposed(x, y, z, 0, 0, 1), isExposed(x, y, z, -1, 0, 0), isExposed(x, y, z, -1, 0, 1));
            ao[3] = calculateAO(isExposed(x, y, z, 0, 0, 1), isExposed(x, y, z, 1, 0, 0), isExposed(x, y, z, 1, 0, 1));

            while (z + extentY < CHUNK_SIZE && blockTypes[getIndex(x, y, z + extentY)] == blockType &&
                !processed[x][y][z + extentY].bottom && isExposed(x, y, z + extentY, 0, -1, 0)) {
                processed[x][y][z + extentY].bottom = true;
                extentY++;
            }
            extentX = 1;
            while (x + extentX < CHUNK_SIZE) {
                bool canExtend = true;
                for (GLint dz = 0; dz < extentY; ++dz) {
                    if (blockTypes[getIndex(x + extentX, y, z + dz)] != blockType ||
                        processed[x + extentX][y][z + dz].bottom ||
                        !isExposed(x + extentX, y, z + dz, 0, -1, 0)) {
                        canExtend = false;
                        break;
                    }
                }
                if (!canExtend) break;
                for (GLint dz = 0; dz < extentY; ++dz) {
                    processed[x + extentX][y][z + dz].bottom = true;
                }
                extentX++;
            }
            Block::addBottomFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, lightLevel, ao);
            break;
        }
        };

    for (GLint x = 0; x < CHUNK_SIZE; ++x) {
        for (GLint y = 0; y < CHUNK_HEIGHT; ++y) {
            for (GLint z = 0; z < CHUNK_SIZE; ++z) {
                GLint index = getIndex(x, y, z);
                if (blockTypes[index] == -1) continue;

                if (!processed[x][y][z].back && isExposed(x, y, z, 0, 0, -1)) processFace(x, y, z, 0);
                if (!processed[x][y][z].front && isExposed(x, y, z, 0, 0, 1)) processFace(x, y, z, 1);
                if (!processed[x][y][z].left && isExposed(x, y, z, -1, 0, 0)) processFace(x, y, z, 2);
                if (!processed[x][y][z].right && isExposed(x, y, z, 1, 0, 0)) processFace(x, y, z, 3);
                if (!processed[x][y][z].top && isExposed(x, y, z, 0, 1, 0)) processFace(x, y, z, 4);
                if (!processed[x][y][z].bottom && isExposed(x, y, z, 0, -1, 0)) processFace(x, y, z, 5);
            }
        }
    }
}

GLint Chunk::getBlockType(GLint x, GLint y, GLint z) const {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
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
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        return;
    }
    GLint index = x * CHUNK_HEIGHT * CHUNK_SIZE + y * CHUNK_SIZE + z;
    blockTypes[index] = type;
}

void Chunk::Draw()
{
    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
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

void Chunk::calculateBounds() {
    minBounds = glm::vec3(chunkX * CHUNK_SIZE, 0, chunkZ * CHUNK_SIZE);
    maxBounds = glm::vec3((chunkX + 1) * CHUNK_SIZE, CHUNK_HEIGHT, (chunkZ + 1) * CHUNK_SIZE);
}

bool Chunk::isInFrustum(const Frustum& frustum) const {
    return frustum.isBoxInFrustum(minBounds, maxBounds);
}