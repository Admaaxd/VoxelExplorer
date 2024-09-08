#pragma once

#include <array>
#include <glm/glm.hpp>
#include <iostream>

class Frustum {
public:
    enum Plane { LEFT, RIGHT, TOP, BOTTOM, PLANE_NEAR, PLANE_FAR };

    // Update the frustum with a new view-projection matrix
    void update(const glm::mat4& viewProjection) {
        auto extractPlane = [&](Plane p, int row, float sign) {
            planes[p].x = viewProjection[0][3] + sign * viewProjection[0][row];
            planes[p].y = viewProjection[1][3] + sign * viewProjection[1][row];
            planes[p].z = viewProjection[2][3] + sign * viewProjection[2][row];
            planes[p].w = viewProjection[3][3] + sign * viewProjection[3][row];
        };

        extractPlane(LEFT, 0, 1);
        extractPlane(RIGHT, 0, -1);
        extractPlane(TOP, 1, -1);
        extractPlane(BOTTOM, 1, 1);
        extractPlane(PLANE_NEAR, 2, 1);
        extractPlane(PLANE_FAR, 2, -1);

        normalizePlanes();
    }

    // Check if AABB is inside the frustum
    bool isBoxInFrustum(const glm::vec3& min, const glm::vec3& max) const {
        for (const auto& plane : planes) {
            glm::vec3 positive = min;
            if (plane.x >= 0) positive.x = max.x;
            if (plane.y >= 0) positive.y = max.y;
            if (plane.z >= 0) positive.z = max.z;

            if (glm::dot(glm::vec3(plane), positive) + plane.w < 0) {
                return false;
            }
        }
        return true;
    }

private:
    std::array<glm::vec4, 6> planes;

    // Normalize the planes
    void normalizePlanes() {
        for (auto& plane : planes) {
            float length = glm::length(glm::vec3(plane));
            if (length > 0.0f) {
                plane /= length;
            }
            else {
                std::cerr << "Warning: Zero-length plane vector detected." << std::endl;
                return;
            }
        }
    }
};