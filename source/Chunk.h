#pragma once

#include "Block.h"
#include "FastNoiseLite.h"
#include "TextureManager.h"

constexpr GLint CHUNK_SIZE = 16;
constexpr GLint CHUNK_HEIGHT = 128;
constexpr GLint WATERLEVEL = 62;

class Chunk
{
public:
	Chunk(int16_t x, int16_t z, TextureManager& textureManager);
	~Chunk();

	void Draw();
	void setupChunk();
	void updateOpenGLBuffers();

	void generateMesh(const std::vector<GLint>& blockTypes);

	GLfloat chunkX, chunkZ;
	std::vector<GLint> blockTypes;

private:
	void generateChunk();

	GLuint textureID;
	FastNoiseLite noiseGenerator;
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	GLuint VAO, VBO, EBO;

	std::vector<std::vector<std::vector<GLint>>> chunkData;

	TextureManager& textureManager;
};