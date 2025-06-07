#ifndef RAYGUN_VK_WINDOW_H
#define RAYGUN_VK_WINDOW_H

#include "GLFW/glfw3.h"

namespace raymarcher::window {
    class Window {
    private:
        GLFWwindow* glfwWindow = nullptr;
        int width = 0, height = 0;

    public:
        Window() = default;
        Window(int width, int height);

        [[nodiscard]] int getWidth() const;
        [[nodiscard]] int getHeight() const;
        [[nodiscard]] GLFWwindow* getGlfwWindow() const;

        [[nodiscard]] bool isMinimized() const;
        [[nodiscard]] bool shouldClose() const;

        [[nodiscard]] bool keyPressed(int glfwKey) const;

        void destroy();
    };
}

#endif //RAYGUN_VK_WINDOW_H
