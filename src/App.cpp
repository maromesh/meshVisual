#include "meshvisual/App.hpp"

int App::run() {
    const bool windowCreated = m_window.create("meshVisual", 1280, 720);
    if (!windowCreated) {
        return 1;
    }

    const bool rendererInitialized = m_renderer.initialize();
    if (!rendererInitialized) {
        return 1;
    }

    while (m_window.isOpen()) {
        m_window.pollEvents();

        m_renderer.beginFrame(
            m_window.framebufferWidth(),
            m_window.framebufferHeight()
        );
        m_renderer.clear(0.10f, 0.00f, 0.05f, 1.0f);
        m_renderer.drawPoints();

        m_window.swapBuffers();
    }

    m_window.destroy();
    return 0;
}