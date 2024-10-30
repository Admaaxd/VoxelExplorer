#include "LoadingScreen.h"

LoadingScreen::LoadingScreen(GLint screenWidth, GLint screenHeight) : screenWidth(screenWidth), screenHeight(screenHeight) {}

void LoadingScreen::display(GLFWwindow* window, World& world, Frustum& frustum, const glm::vec3& camPos) {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    while (!world.isInitialChunksLoaded()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2((screenWidth - 300) / 2, (screenHeight - 300) / 2), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Always);

        ImGui::Begin("Loading...", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDecoration);

        ImGui::BeginChild("CanvasRegion", ImVec2(300, 300), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDecoration);
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasCenter = ImVec2(canvasPos.x + 150, canvasPos.y + 150);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        GLint index = 0;

        for (const auto& pair : world.getChunks()) {
            const Chunk* chunk = pair.second;
            ImVec2 chunkPosMin = ImVec2(canvasCenter.x + (chunk->getMinBounds().x - camPos.x) * 0.7f, canvasCenter.y + (chunk->getMinBounds().z - camPos.z) * 0.7f);
            ImVec2 chunkPosMax = ImVec2(canvasCenter.x + (chunk->getMaxBounds().x - camPos.x) * 0.7f, canvasCenter.y + (chunk->getMaxBounds().z - camPos.z) * 0.7f);

            ImU32 fillColor;
            if ((index % 8) < 2) {
                fillColor = IM_COL32(124, 252, 0, 150); // green
            }
            else if ((index % 8) < 6) {
                fillColor = IM_COL32(123, 63, 0, 150); // brown
            }
            else if ((index % 8) < 7) {
                fillColor = IM_COL32(211, 211, 211, 150); // gray
            }
            else {
                fillColor = IM_COL32(0, 255, 240, 150); // blue
            }

            drawList->AddRectFilled(chunkPosMin, chunkPosMax, fillColor);
            drawList->AddRect(chunkPosMin, chunkPosMax, IM_COL32(0, 0, 0, 255));

            index++;
        }

        ImGui::EndChild();
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();

        world.processChunkLoadQueue(1, 1);
    }
}