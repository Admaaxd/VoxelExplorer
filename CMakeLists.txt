cmake_minimum_required(VERSION 3.16)
project(VoxelExplorer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES source/*.cpp source/*.h)
add_executable(VoxelExplorer ${SOURCES})

# glfw
add_subdirectory(thirdparty/include/GLFW EXCLUDE_FROM_ALL)

# glm
include_directories(thirdparty/include/glm)

# glad
add_subdirectory(thirdparty/include/glad EXCLUDE_FROM_ALL)

add_library(imgui EXCLUDE_FROM_ALL
    thirdparty/imgui/imconfig.h
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui.h
    thirdparty/imgui/imgui_demo.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_impl_glfw.cpp
    thirdparty/imgui/imgui_impl_glfw.h
    thirdparty/imgui/imgui_impl_opengl3.cpp
    thirdparty/imgui/imgui_impl_opengl3.h
    thirdparty/imgui/imgui_impl_opengl3_loader.h
    thirdparty/imgui/imgui_internal.h
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/imstb_rectpack.h
    thirdparty/imgui/imstb_textedit.h
    thirdparty/imgui/imstb_truetype.h
)
include_directories(imgui thirdparty/imgui)

# include FastNoiseLite
include_directories(VoxelExplorer thirdparty/FastNoiseLite)

# stb
include_directories(thirdparty/stb)
add_library(stb STATIC thirdparty/stb/stb.cpp)
target_include_directories(stb PRIVATE thirdparty/stb)

# link glfw to imgui and link everything to the VoxelExplorer app
target_link_libraries(imgui PRIVATE glfw)
target_link_libraries(VoxelExplorer PRIVATE glfw glad imgui stb)

# shader
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/shaders")

add_definitions(-DSHADER_DIR="${SHADER_OUTPUT_DIR}")

# Symlink shaders directory (cross-platform support)
if (UNIX)
    add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/shaders ${SHADER_OUTPUT_DIR}
    )
else()
    add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders ${SHADER_OUTPUT_DIR}
    )
endif()

# textures
set(TEXTURES_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/textures")

add_definitions(-DTEXTURES_DIR="${TEXTURES_OUTPUT_DIR}")

# Symlink textures directory (cross-platform support)
if (UNIX)
    add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/textures ${TEXTURES_OUTPUT_DIR}
    )
else()
    add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/textures ${TEXTURES_OUTPUT_DIR}
    )
endif()

# skybox
set(SKYBOX_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/skybox")

add_definitions(-DSKYBOX_DIR="${SKYBOX_OUTPUT_DIR}")

# Symlink skybox directory (cross-platform support)
if (UNIX)
    add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/skybox ${SKYBOX_OUTPUT_DIR}
    )
else()
    add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/skybox ${SKYBOX_OUTPUT_DIR}
    )
endif()