#pragma once

#include <string>

struct GLFWwindow;

class Window {
public:
    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    bool create(const std::string& title, int width, int height);
    void pollEvents();
    void swapBuffers();
    void close();
    void destroy();

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] int width() const;
    [[nodiscard]] int height() const;
    [[nodiscard]] const std::string& title() const;
    [[nodiscard]] GLFWwindow* nativeHandle() const;

private:
    GLFWwindow* m_window {nullptr};
    bool m_isOpen {false};
    int m_width {0};
    int m_height {0};
    std::string m_title {};
    bool m_glfwInitialised {false};
};