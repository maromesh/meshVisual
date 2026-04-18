#pragma once

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
    unsigned int m_pointProgram {0};
    unsigned int m_pointVao {0};
    unsigned int m_pointVbo {0};
    int m_pointCount {0};
    bool m_initialized {false};
};
