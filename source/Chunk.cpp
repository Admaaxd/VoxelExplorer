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

    sunlitBlocks.resize(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE, false);

    uint8_t caveMinHeight = CHUNK_HEIGHT / 64;
    uint8_t surfaceBuffer = CHUNK_HEIGHT / 10;

    for (GLint x = 0; x < CHUNK_SIZE; ++x)
    {
        for (GLint z = 0; z < CHUNK_SIZE; ++z)
        {
            bool isSunlit = true;
            GLfloat worldX = static_cast<GLfloat>(chunkX * CHUNK_SIZE + x);
            GLfloat worldZ = static_cast<GLfloat>(chunkZ * CHUNK_SIZE + z);

            // Generate base terrain height
            GLfloat baseHeight = baseNoise.GetNoise(worldX, worldZ);
            GLfloat elevation = elevationNoise.GetNoise(worldX, worldZ);

            uint8_t terrainHeight = static_cast<uint8_t>((baseHeight + elevation * 0.5f + 1.0f) * 0.5f * (CHUNK_HEIGHT - 30)) + 30;

            for (GLint y = CHUNK_HEIGHT - 1; y >= 0; --y)
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

                if (isSunlit) {
                    sunlitBlocks[index] = true;
                    isSunlit = false;
                }
                else {
                    sunlitBlocks[index] = false;
                }
            }
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

    auto processFace = [&](GLint x, GLint y, GLint z, int8_t face, bool isSunlit) {
        GLint extentX = 1;
        GLint extentY = 1;
        GLint blockType = blockTypes[getIndex(x, y, z)];
        GLint textureLayer = getTextureLayer(blockType, face);

        switch (face) {
        case 0: // Back face

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
            Block::addBackFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, isSunlit);
            break;

        case 1: // Front face

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
            Block::addFrontFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, isSunlit);
            break;

        case 2: // Left face

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
            Block::addLeftFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, isSunlit);
            break;

        case 3: // Right face

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
            Block::addRightFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, isSunlit);
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
            Block::addTopFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, isSunlit);
            break;

        case 5: // Bottom face

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
            Block::addBottomFace(vertices, indices, vertexOffset, chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, extentX, extentY, textureLayer, isSunlit);
            break;
        }
        };

    for (GLint x = 0; x < CHUNK_SIZE; ++x) {
        for (GLint y = 0; y < CHUNK_HEIGHT; ++y) {
            for (GLint z = 0; z < CHUNK_SIZE; ++z) {
                GLint index = getIndex(x, y, z);
                bool isSunlit = sunlitBlocks[index];
                if (blockTypes[index] == -1) continue;

                if (!processed[x][y][z].back && isExposed(x, y, z, 0, 0, -1)) processFace(x, y, z, 0, isSunlit);
                if (!processed[x][y][z].front && isExposed(x, y, z, 0, 0, 1)) processFace(x, y, z, 1, isSunlit);
                if (!processed[x][y][z].left && isExposed(x, y, z, -1, 0, 0)) processFace(x, y, z, 2, isSunlit);
                if (!processed[x][y][z].right && isExposed(x, y, z, 1, 0, 0)) processFace(x, y, z, 3, isSunlit);
                if (!processed[x][y][z].top && isExposed(x, y, z, 0, 1, 0)) processFace(x, y, z, 4, isSunlit);
                if (!processed[x][y][z].bottom && isExposed(x, y, z, 0, -1, 0)) processFace(x, y, z, 5, isSunlit);
            }
        }
    }
}

void Chunk::updateSunlightColumn(GLint localX, GLint localZ)
{
    bool isSunlit = true;
    for (GLint y = CHUNK_HEIGHT - 1; y >= 0; --y)
    {
        GLint index = localX * CHUNK_HEIGHT * CHUNK_SIZE + y * CHUNK_SIZE + localZ;

        if (blockTypes[index] == -1)
        {
            continue;
        }

        if (isSunlit)
        {
            sunlitBlocks[index] = true;
            isSunlit = false;
        }
        else
        {
            sunlitBlocks[index] = false;
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
    updateSunlightColumn(x, z);
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Texture layer attribute
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    
    // Normal attribute
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    // Sunlit flag attribute
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (void*)(9 * sizeof(GLfloat)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);
}

void Chunk::calculateBounds() {
    minBounds = glm::vec3(chunkX * CHUNK_SIZE, 0, chunkZ * CHUNK_SIZE);
    maxBounds = glm::vec3((chunkX + 1) * CHUNK_SIZE, CHUNK_HEIGHT, (chunkZ + 1) * CHUNK_SIZE);
}

bool Chunk::isInFrustum(const Frustum& frustum) const {
    return frustum.isBoxInFrustum(minBounds, maxBounds);
}