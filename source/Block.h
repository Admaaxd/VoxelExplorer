#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <vector>
#include <array>

class Block
{
public:
	static void addFrontFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4]);
	static void addBackFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4]);
	static void addTopFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4]);
	static void addBottomFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentX, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4]);
	static void addLeftFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentZ, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4]);
	static void addRightFace(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices, GLint& vertexOffset, int16_t x, int16_t y, int16_t z, uint8_t extentZ, uint8_t extentY, uint8_t textureLayer, uint8_t lightLevel, const GLfloat ao[4]);

private:
	GLfloat x, y, z;
};