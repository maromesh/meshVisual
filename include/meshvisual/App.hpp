#pragma once

#include "meshvisual/core/Renderer.hpp"
#include "meshvisual/core/Window.hpp"
#include "meshvisual/graph/Graph.hpp"
#include "meshvisual/graph/GraphGenerator.hpp"

#include <chrono>
#include <vector>

class App {
public:
    int run();

private:
    struct Viewport {
        float centerX {0.0f};
        float centerY {0.0f};
        float width {0.0f};
        float height {0.0f};
    };

    [[nodiscard]] GraphGenerationConfig createGenerationConfig() const;
    [[nodiscard]] Viewport currentViewport() const;
    void updateSimulation(float deltaSeconds);
    [[nodiscard]] std::vector<float> buildLineVertices(const Viewport& viewport) const;
    [[nodiscard]] std::vector<float> buildPointVertices(const Viewport& viewport) const;
    [[nodiscard]] float worldToClipX(float worldX, const Viewport& viewport) const;
    [[nodiscard]] float worldToClipY(float worldY, const Viewport& viewport) const;

    Window m_window;
    Renderer m_renderer;
    GraphGenerator m_graphGenerator;
    Graph m_graph;
    GraphGenerationConfig m_generationConfig;
    std::chrono::steady_clock::time_point m_lastFrameTime {};
};
