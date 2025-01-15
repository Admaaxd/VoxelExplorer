#include "Block.h"

void Block::addBackFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4])
{
	GLfloat topX = x + extentX, topY = y + extentY, texX = 1.0f * extentX, texY = 1.0f * extentY;

	GLfloat nx = 0.0f, ny = 0.0f, nz = -1.0f;

	GLfloat normalizedLightLevel = lightLevel / 15.0f;

	std::array<GLfloat, 44> faceVertices = 
	{
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[0],
		topX,						static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	texX,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[1],
		topX,						topY,						static_cast<GLfloat>(z),	texX,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[2],
		static_cast<GLfloat>(x),	topY,						static_cast<GLfloat>(z),	0.0f,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[3]
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		static_cast<uint16_t>(vertexOffset),		static_cast<uint16_t>(vertexOffset) + 1,	static_cast<uint16_t>(vertexOffset) + 2,
		static_cast<uint16_t>(vertexOffset) +2,		static_cast<uint16_t>(vertexOffset) + 3,	static_cast<uint16_t>(vertexOffset)
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;
}

void Block::addFrontFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4])
{
	GLfloat topX = x + extentX, topY = y + extentY, texX = 1.0f * extentX, texY = 1.0f * extentY;

	GLfloat nx = 0.0f, ny = 0.0f, nz = 1.0f;

	GLfloat normalizedLightLevel = lightLevel / 15.0f;

	std::array<GLfloat, 44> faceVertices = {
		static_cast<GLfloat>(x),	topY,						static_cast<GLfloat>(z + 1),	0.0f,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[0],
		topX,						topY,						static_cast<GLfloat>(z + 1),	texX,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[1],
		topX,						static_cast<GLfloat>(y),	static_cast<GLfloat>(z + 1),	texX,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[2],
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	static_cast<GLfloat>(z + 1),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[3]
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		static_cast<uint16_t>(vertexOffset),		static_cast<uint16_t>(vertexOffset) + 1,	static_cast<uint16_t>(vertexOffset) + 2,
		static_cast<uint16_t>(vertexOffset) + 2,	static_cast<uint16_t>(vertexOffset) + 3,	static_cast<uint16_t>(vertexOffset)
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;
}

void Block::addTopFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4])
{
	GLfloat topX = x + extentX, topZ = z + extentY, texX = 1.0f * extentX, texY = 1.0f * extentY;

	GLfloat nx = 0.0f, ny = 1.0f, nz = 0.0f;

	GLfloat normalizedLightLevel = lightLevel / 15.0f;

	std::array<GLfloat, 44> faceVertices = 
	{
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y + 1), static_cast<GLfloat>(z),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[0],
		topX,						static_cast<GLfloat>(y + 1), static_cast<GLfloat>(z),	texX,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[1],
		topX,						static_cast<GLfloat>(y + 1), topZ,						texX,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[2],
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y + 1), topZ,						0.0f,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[3]
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		static_cast<uint16_t>(vertexOffset),		static_cast<uint16_t>(vertexOffset) + 1,	static_cast<uint16_t>(vertexOffset) + 2,
		static_cast<uint16_t>(vertexOffset) + 2,	static_cast<uint16_t>(vertexOffset) + 3,	static_cast<uint16_t>(vertexOffset)
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;

}

void Block::addBottomFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4])
{
	GLfloat topX = x + extentX, topZ = z + extentY, texX = 1.0f * extentX, texY = 1.0f * extentY;

	GLfloat nx = 0.0f, ny = -1.0f, nz = 0.0f;

	GLfloat normalizedLightLevel = lightLevel / 15.0f;

	std::array<GLfloat, 44> faceVertices = 
	{
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	topZ,						0.0f,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[0],
		topX,						static_cast<GLfloat>(y),	topZ,						texX,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[1],
		topX,						static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	texX,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[2],
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[3]
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		static_cast<uint16_t>(vertexOffset),		static_cast<uint16_t>(vertexOffset) + 1,	static_cast<uint16_t>(vertexOffset) + 2,
		static_cast<uint16_t>(vertexOffset) + 2,	static_cast<uint16_t>(vertexOffset) + 3,	static_cast<uint16_t>(vertexOffset)
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;

}

void Block::addLeftFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentZ, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4])
{
	GLfloat topZ = z + extentZ, topY = y + extentY, texZ = 1.0f * extentZ, texY = 1.0f * extentY;

	GLfloat nx = -1.0f, ny = 0.0f, nz = 0.0f;

	GLfloat normalizedLightLevel = lightLevel / 15.0f;

	std::array<GLfloat, 44> faceVertices = 
	{
		static_cast<GLfloat>(x),	topY,						topZ,						texZ,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[0],
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	topZ,						texZ,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[1],
		static_cast<GLfloat>(x),	static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[2],
		static_cast<GLfloat>(x),	topY,						static_cast<GLfloat>(z),	0.0f,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[3]
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		static_cast<uint16_t>(vertexOffset),		static_cast<uint16_t>(vertexOffset) + 1,	static_cast<uint16_t>(vertexOffset) + 2,
		static_cast<uint16_t>(vertexOffset) + 2,	static_cast<uint16_t>(vertexOffset) + 3,	static_cast<uint16_t>(vertexOffset)
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;

}

void Block::addRightFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentZ, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4])
{
	GLfloat topZ = z + extentZ, topY = y + extentY, texZ = 1.0f * extentZ, texY = 1.0f * extentY;

	GLfloat nx = 1.0f, ny = 0.0f, nz = 0.0f;

	GLfloat normalizedLightLevel = lightLevel / 15.0f;

	std::array<GLfloat, 44> faceVertices = 
	{
		static_cast<GLfloat>(x + 1),	static_cast<GLfloat>(y),	topZ,						texZ,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[0],
		static_cast<GLfloat>(x + 1),	topY,						topZ,						texZ,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[1],
		static_cast<GLfloat>(x + 1),	topY,						static_cast<GLfloat>(z),	0.0f,	texY,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[2],
		static_cast<GLfloat>(x + 1),	static_cast<GLfloat>(y),	static_cast<GLfloat>(z),	0.0f,	0.0f,	static_cast<GLfloat>(textureLayer), nx, ny, nz, normalizedLightLevel, ao[3]
	};

	vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

	std::array<uint16_t, 6> faceIndices = 
	{
		static_cast<uint16_t>(vertexOffset),		static_cast<uint16_t>(vertexOffset) + 1,	static_cast<uint16_t>(vertexOffset) + 2,
		static_cast<uint16_t>(vertexOffset) + 2,	static_cast<uint16_t>(vertexOffset) + 3,	static_cast<uint16_t>(vertexOffset)
	};

	indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());

	vertexOffset += 4;

}
