#include "Window.h"

#include <stdexcept>

raymarcher::window::Window::Window(int width, int height) {
    if (glfwInit() != GLFW_TRUE) {  // todo: should glfw init for every object or just once?
        throw std::runtime_error("Cannot init GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glfwWindow = glfwCreateWindow(width, height, "Raymarcher", nullptr, nullptr);

    glfwGetWindowSize(glfwWindow, &this->width, &this->height);
}

int raymarcher::window::Window::getWidth() const {
    return width;
}

int raymarcher::window::Window::getHeight() const {
    return height;
}

void raymarcher::window::Window::destroy() {
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();  // todo: verify if this is best practice.
}

GLFWwindow* raymarcher::window::Window::getGlfwWindow() const {
    return glfwWindow;
}

bool raymarcher::window::Window::shouldClose() const {
    return glfwWindowShouldClose(glfwWindow);
}

bool raymarcher::window::Window::isMinimized() const {
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(glfwWindow, &fbWidth, &fbHeight);
    return fbWidth == 0 || fbHeight == 0;
}

bool raymarcher::window::Window::keyPressed(int glfwKey) const {
    return glfwGetKey(glfwWindow, glfwKey) == GLFW_PRESS;
}
