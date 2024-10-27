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
#include <windows.h>
#include <psapi.h>
#include <iostream>

#include "shader.h"
#include "Camera.h"
#include "World.h"
#include "Player.h"
#include "Crosshair.h"
#include "blockOutline.h"
#include "SkyboxRenderer.h"

class main
{
public:
	static void processRendering(GLFWwindow* window, shader& mainShader, shader& waterShader, shader& meshingShader, shader& crosshairShader, SkyboxRenderer& skybox,
		Player& player, Frustum& frustum, World& world, Crosshair& crosshair, BlockOutline& blockOutline);

	static void initializeGLFW(GLFWwindow*& window);
	static void initializeGLAD();
	static void framebuffer_size_callback(GLFWwindow* window, GLint width, GLint height);
	static void setupRenderingState();

	static void displayLoadingScreen(GLFWwindow* window, World& world, Frustum& frustum);

	static void updateFPS();

	static void initializeMeshOutline(shader& meshingShader, glm::mat4 model, glm::mat4 view, glm::mat4 projection, World& world, Frustum& frustum);

	static void renderSkybox(SkyboxRenderer& skybox, glm::mat4& view, const glm::mat4& projection, Camera& camera, Player& player);

	static void renderBlockOutline(const Player& player, const glm::mat4& projection, const glm::mat4& view, BlockOutline& blockOutline);
	static void initializeImGui(GLFWwindow* window);
	static void renderImGui(GLFWwindow* window, const glm::vec3& playerPosition, Player& player, World& world, Frustum& frustum, SkyboxRenderer& skyboxRenderer);
	static void cleanupImGui();
	static void cleanup(shader& mainShader, shader& meshingShader, Crosshair& crosshair, shader& waterShader);
	static void mouse_callback(GLFWwindow* window, GLdouble xposIn, GLdouble yposIn);
	static void mouseButtonCallback(GLFWwindow* window, GLint button, GLint action, GLint mods);
	static size_t getCurrentMemoryUsage();
};