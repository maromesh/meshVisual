#include "meshvisual/core/Window.hpp"

#include <GLFW/glfw3.h>
#include <iostream>

Window::~Window() {
    destroy();
}

bool Window::create(const std::string& title, int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    if (!glfwInit()) {
        return false;
    }

    m_glfwInitialized = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (m_window == nullptr) {
        destroy();
        return false;
    }

    glfwMakeContextCurrent(m_window);

    m_title = title;
    m_width = width;
    m_height = height;

    return true;
}

void Window::pollEvents() {
    glfwPollEvents();
}   

void Window::swapBuffers() {
    if (m_window != nullptr) {
        glfwSwapBuffers(m_window);
    }
}

void Window::close() {
    if (m_window != nullptr) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

void Window::destroy() {
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    if (m_glfwInitialized) {
        glfwTerminate();
        m_glfwInitialized = false;
    }

    m_width = 0;
    m_height = 0;
    m_title.clear();
}

bool Window::isOpen() const {
    return m_window != nullptr && glfwWindowShouldClose(m_window) == GLFW_FALSE;
}

int Window::width() const {
    return m_width;
}

int Window::height() const {
    return m_height;
}

const std::string& Window::title() const {
    return m_title;
}

GLFWwindow* Window::nativeHandle() const{
    return m_window;
}

int Window::framebufferWidth() const {
    if (m_window == nullptr) {
        return 0;
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);
    return framebufferWidth;
}

int Window::framebufferHeight() const {
    if (m_window == nullptr) {
        return 0;
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);
    return framebufferHeight;
}