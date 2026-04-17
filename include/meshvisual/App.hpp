#pragma once

#include "meshvisual/core/Renderer.hpp"
#include "meshvisual/core/Window.hpp"

class App {
public:
    int run();

private:
    Window m_window;
    Renderer m_renderer;
};