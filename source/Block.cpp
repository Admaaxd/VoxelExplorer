#include "Block.h"

void Block::addBackFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer)
{
	GLfloat topX = x + extentX, topY = y + extentY, texX = 1.0f * extentX, texY = 1.0f * extentY;

	std::array<GLfloat, 24> faceVertices = 
	{
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer),
		topX,						static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	texX,	0.0f,	static_cast<GLfloat>(textureLayer),
		topX,						topY,						static_cast<GLfloat>(z),	texX,	texY,	static_cast<GLfloat>(textureLayer),
		static_cast<GLfloat>(x),	topY,						static_cast<GLfloat>(z),	0.0f,	texY,	static_cast<GLfloat>(textureLayer)
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		vertexOffset,		vertexOffset + 1,	vertexOffset + 2,
		vertexOffset +2,	vertexOffset + 3,	vertexOffset
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;
}

void Block::addFrontFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer)
{
	GLfloat topX = x + extentX, topY = y + extentY;
	GLfloat texX = 1.0f * extentX;
	GLfloat texY = 1.0f * extentY;

	std::array<GLfloat, 24> faceVertices = {
		static_cast<GLfloat>(x),	topY,						static_cast<GLfloat>(z + 1),	0.0f,	texY,	static_cast<GLfloat>(textureLayer),
		topX,						topY,						static_cast<GLfloat>(z + 1),	texX,	texY,	static_cast<GLfloat>(textureLayer),
		topX,						static_cast<GLfloat>(y),	static_cast<GLfloat>(z + 1),	texX,	0.0f,	static_cast<GLfloat>(textureLayer),
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	static_cast<GLfloat>(z + 1),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer)
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		vertexOffset,		vertexOffset + 1,	vertexOffset + 2,
		vertexOffset + 2,	vertexOffset + 3,	vertexOffset
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;
}

void Block::addTopFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer)
{
	GLfloat topX = x + extentX, topZ = z + extentY, texX = 1.0f * extentX, texY = 1.0f * extentY;

	std::array<GLfloat, 24> faceVertices = 
	{
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y + 1), static_cast<GLfloat>(z),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer),
		topX,						static_cast<GLfloat>(y + 1), static_cast<GLfloat>(z),	texX,	0.0f,	static_cast<GLfloat>(textureLayer),
		topX,						static_cast<GLfloat>(y + 1), topZ,						texX,	texY,	static_cast<GLfloat>(textureLayer),
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y + 1), topZ,						0.0f,	texY,	static_cast<GLfloat>(textureLayer)
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		vertexOffset,		vertexOffset + 1,	vertexOffset + 2,
		vertexOffset + 2,	vertexOffset + 3,	vertexOffset
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;

}

void Block::addBottomFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer)
{
	GLfloat topX = x + extentX, topZ = z + extentY, texX = 1.0f * extentX, texY = 1.0f * extentY;

	std::array<GLfloat, 24> faceVertices = 
	{
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	topZ,						0.0f,	texY,	static_cast<GLfloat>(textureLayer),
		topX,						static_cast<GLfloat>(y),	topZ,						texX,	texY,	static_cast<GLfloat>(textureLayer),
		topX,						static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	texX,	0.0f,	static_cast<GLfloat>(textureLayer),
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer)
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		vertexOffset,		vertexOffset + 1,	vertexOffset + 2,
		vertexOffset + 2,	vertexOffset + 3,	vertexOffset
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;

}

void Block::addLeftFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentZ, uint8_t extentY, uint8_t textureLayer)
{
	GLfloat topZ = z + extentZ, topY = y + extentY, texZ = 1.0f * extentZ, texY = 1.0f * extentY;

	std::array<GLfloat, 24> faceVertices = 
	{
		static_cast<GLfloat>(x),	topY,						topZ,						texZ,	texY,	static_cast<GLfloat>(textureLayer),
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	topZ,						texZ,	0.0f,	static_cast<GLfloat>(textureLayer),
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer),
		static_cast<GLfloat>(x),	topY,						static_cast<GLfloat>(z),	0.0f,	texY,	static_cast<GLfloat>(textureLayer)
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		vertexOffset,		vertexOffset + 1,	vertexOffset + 2,
		vertexOffset + 2,	vertexOffset + 3,	vertexOffset
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;

}

void Block::addRightFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentZ, uint8_t extentY, uint8_t textureLayer)
{
	GLfloat topZ = z + extentZ, topY = y + extentY, texZ = 1.0f * extentZ, texY = 1.0f * extentY;

	std::array<GLfloat, 24> faceVertices = 
	{
		static_cast<GLfloat>(x + 1),	static_cast<GLfloat>(y),	topZ,						texZ,	0.0f,	static_cast<GLfloat>(textureLayer),
		static_cast<GLfloat>(x + 1),	topY,						topZ,						texZ,	texY,	static_cast<GLfloat>(textureLayer),
		static_cast<GLfloat>(x + 1),	topY,						static_cast<GLfloat>(z),	0.0f,	texY,	static_cast<GLfloat>(textureLayer),
		static_cast<GLfloat>(x + 1),	static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer)
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		vertexOffset,		vertexOffset + 1,	vertexOffset + 2,
		vertexOffset + 2,	vertexOffset + 3,	vertexOffset
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;

}
