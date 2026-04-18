#pragma once

#include "meshvisual/graph/GraphGenerator.hpp"

#include <string>

struct ColorRgba {
    float red {0.0f};
    float green {0.0f};
    float blue {0.0f};
    float alpha {1.0f};
};

struct AppSettings {
    int windowWidth {1280};
    int windowHeight {720};
    float pointSize {10.0f};
    float lineWidth {1.0f};
    bool shapesEnabled {true};
    ColorRgba backgroundColor {0.10f, 0.00f, 0.05f, 1.0f};
    ColorRgba pointColor {0.90f, 0.95f, 1.00f, 1.0f};
    ColorRgba edgeColor {0.35f, 0.70f, 0.95f, 1.0f};
    GraphGenerationConfig graph {};
};

class Settings {
public:
    static AppSettings loadAppSettings(const std::string& path);
};
