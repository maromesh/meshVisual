#include "meshvisual/App.hpp"
#include <iostream>

int App::run() {
    int frameCount = 0;

    while (m_running) {
        ++frameCount;
        std::cout << "Frame: " << frameCount << "\n";

        if (frameCount >= 1) {
            m_running = false;
        }
    }

    return 0;
}