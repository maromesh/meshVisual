#pragma once

#include "meshvisual/core/Settings.hpp"

#include <span>
#include <cstdint>

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    bool initialize();
    void configure(const AppSettings& settings);
    void beginFrame(int width, int height);
    void clear(float red, float green, float blue, float alpha);
    void uploadLines(std::span<const float> lineVertices);
    void uploadPoints(std::span<const float> pointVertices);
    void drawLines();
    void drawPoints();
    void destroy();

private:
    [[nodiscard]] unsigned int compileShader(unsigned int type, const char* source) const;
    [[nodiscard]] bool createLinePipeline();
    [[nodiscard]] bool createPointPipeline();

private:
    unsigned int m_lineProgram {0};
    unsigned int m_lineVao {0};
    unsigned int m_lineVbo {0};
    int m_lineVertexCount {0};
    float m_lineWidth {1.0f};
    ColorRgba m_edgeColor {0.35f, 0.70f, 0.95f, 1.0f};
    unsigned int m_pointProgram {0};
    unsigned int m_pointVao {0};
    unsigned int m_pointVbo {0};
    int m_pointCount {0};
    float m_pointSize {10.0f};
    ColorRgba m_pointColor {0.90f, 0.95f, 1.00f, 1.0f};
    bool m_initialized {false};
};
