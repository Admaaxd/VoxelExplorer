#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stb/stb_image.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

#include "shader.h"
#include "Camera.h"
#include "World.h"

class main
{
public:
	static void initializeGLFW(GLFWwindow*& window);
	static void initializeGLAD();
	static void framebuffer_size_callback(GLFWwindow* window, GLint width, GLint height);
	static void updateFPS();
	static void initializeImGui(GLFWwindow* window);
	static void renderImGui(GLFWwindow* window, const glm::vec3& playerPosition);
	static void cleanupImGui();
	static void processInput(GLFWwindow* window);
	static void mouse_callback(GLFWwindow* window, GLdouble xposIn, GLdouble yposIn);
};