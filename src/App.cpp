#include "meshvisual/App.hpp"
#include <iostream>

int App::run() {
    const bool created = m_window.create("meshVisual", 1280, 720);
    if (!created) {
        return 1;
    }

    while (m_window.isOpen()) {
        m_window.pollEvents();
        m_window.swapBuffers();
    }

    m_window.destroy();
    return 0;
}