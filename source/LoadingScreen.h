#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "World.h"

class LoadingScreen {
public:
    LoadingScreen(GLint screenWidth, GLint screenHeight);
    void display(GLFWwindow* window, World& world, Frustum& frustum, const glm::vec3& camPos);

private:
    GLint screenWidth;
    GLint screenHeight;
};
