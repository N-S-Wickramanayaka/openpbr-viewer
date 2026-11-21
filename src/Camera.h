#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

struct OrbitCamera {
    glm::vec3 target{0.0f};
    float yaw = glm::radians(20.0f);
    float pitch = glm::radians(10.0f);
    float radius = 3.0f;
    float fovY = glm::radians(45.0f);
    float sceneRadius = 1.0f;   // size of what we look at; sets the clip planes

    glm::vec3 position() const
    {
        return target + radius * glm::vec3(std::cos(pitch) * std::sin(yaw), std::sin(pitch),
                                           std::cos(pitch) * std::cos(yaw));
    }
    glm::mat4 view() const { return glm::lookAt(position(), target, glm::vec3(0.0f, 1.0f, 0.0f)); }
    glm::mat4 projection(float aspect) const
    {
        return glm::perspective(fovY, aspect, 0.01f * sceneRadius, 100.0f * sceneRadius);
    }

    // Places the camera so a bounding sphere fills the view, with a little margin.
    void frame(const glm::vec3& center, float boundsRadius)
    {
        target = center;
        sceneRadius = boundsRadius;
        radius = 1.1f * boundsRadius / std::sin(0.5f * fovY);
    }
};