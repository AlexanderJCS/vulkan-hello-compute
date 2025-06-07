

#ifndef RAYMARCH_CAMERA_H
#define RAYMARCH_CAMERA_H

#include <glm/glm.hpp>

#include "../window/Window.h"

namespace raymarcher::graphics {
    class Camera {
    public:
        Camera() = default;
        Camera(const raymarcher::window::Window& renderWindow, float fov, float aspectRatio, glm::vec3 pos, glm::vec3 cameraFront);
        Camera(const Camera& other);
        Camera& operator=(const Camera& other);

        /**
         * Process input. Returns if the camera position changed. Important for "clearing" the screen and starting the
         * render fresh when the camera changed.
         * @param window The render preview window.
         * @param timeDelta The delta time between frames in seconds.
         * @return If the camera position changed
         */
        void processInput(const raymarcher::window::Window& window, double timeDelta);

        void toggleInput(GLFWwindow* window);

        static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

        [[nodiscard]] const glm::mat4& getInverseView() const;
        [[nodiscard]] const glm::mat4& getInverseProjection() const;
        [[nodiscard]] bool isAcceptingInput() const;
        [[nodiscard]] bool hasChanged() const;

        void refresh();

    private:
        const raymarcher::window::Window* renderWindow = nullptr;
        bool ignoreNextMouseInput = false;
        bool input = false;
        glm::vec2 lastMousePos = glm::vec2(-1, -1);
        float pitch = 0;
        float yaw = -90.0f;
        bool changed = false;

        glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 inverseView = glm::mat4(1.0f);
        glm::mat4 inverseProjection = glm::mat4(1.0f);
    };
}

#endif //RAYMARCH_CAMERA_H
