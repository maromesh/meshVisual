#include "meshvisual/App.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <unordered_map>
#include <vector>

int App::run() {
    const bool windowCreated = m_window.create("meshVisual", 1280, 720);
    if (!windowCreated) {
        return 1;
    }

    const bool rendererInitialized = m_renderer.initialize();
    if (!rendererInitialized) {
        return 1;
    }

    m_generationConfig = createGenerationConfig();
    m_graph = m_graphGenerator.createRandomGraph(m_generationConfig);
    m_lastFrameTime = std::chrono::steady_clock::now();

    while (m_window.isOpen()) {
        m_window.pollEvents();

        const auto currentTime = std::chrono::steady_clock::now();
        const std::chrono::duration<float> elapsed = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;

        const float deltaSeconds = std::min(elapsed.count(), 0.05f);
        updateSimulation(deltaSeconds);

        const Viewport viewport = currentViewport();
        const std::vector<float> lineVertices = buildLineVertices(viewport);
        const std::vector<float> pointVertices = buildPointVertices(viewport);
        m_renderer.uploadLines(lineVertices);
        m_renderer.uploadPoints(pointVertices);

        m_renderer.beginFrame(
            m_window.framebufferWidth(),
            m_window.framebufferHeight()
        );
        m_renderer.clear(0.10f, 0.00f, 0.05f, 1.0f);
        m_renderer.drawLines();
        m_renderer.drawPoints();

        m_window.swapBuffers();
    }

    m_window.destroy();
    return 0;
}

GraphGenerationConfig App::createGenerationConfig() const {
    GraphGenerationConfig config;
    config.worldWidth = 4800.0f;
    config.worldHeight = 3200.0f;
    config.pointDensity = 0.00002f;
    config.maxConnectionDistance = 260.0f;
    config.maxNeighborsPerNode = 3;
    config.minSpeed = 12.0f;
    config.maxSpeed = 26.0f;
    return config;
}

App::Viewport App::currentViewport() const {
    constexpr float baseViewportHeight = 900.0f;

    const int framebufferWidth = std::max(1, m_window.framebufferWidth());
    const int framebufferHeight = std::max(1, m_window.framebufferHeight());
    const float aspectRatio = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);

    return {
        .centerX = m_generationConfig.worldWidth * 0.5f,
        .centerY = m_generationConfig.worldHeight * 0.5f,
        .width = baseViewportHeight * aspectRatio,
        .height = baseViewportHeight,
    };
}

void App::updateSimulation(float deltaSeconds) {
    if (deltaSeconds <= 0.0f) {
        return;
    }

    bool wrappedNode = false;

    for (Node& node : m_graph.nodes()) {
        node.x += node.velocityX * deltaSeconds;
        node.y += node.velocityY * deltaSeconds;

        if (node.x < 0.0f) {
            node.x += m_generationConfig.worldWidth;
            wrappedNode = true;
        } else if (node.x > m_generationConfig.worldWidth) {
            node.x -= m_generationConfig.worldWidth;
            wrappedNode = true;
        }

        if (node.y < 0.0f) {
            node.y += m_generationConfig.worldHeight;
            wrappedNode = true;
        } else if (node.y > m_generationConfig.worldHeight) {
            node.y -= m_generationConfig.worldHeight;
            wrappedNode = true;
        }
    }

    if (wrappedNode) {
        m_graphGenerator.rebuildEdges(m_graph, m_generationConfig);
    }
}

std::vector<float> App::buildLineVertices(const Viewport& viewport) const {
    std::unordered_map<int, const Node*> nodesById;
    nodesById.reserve(m_graph.nodes().size());

    for (const Node& node : m_graph.nodes()) {
        nodesById.emplace(node.id, &node);
    }

    std::vector<float> lineVertices;
    lineVertices.reserve(m_graph.edges().size() * 4U);

    for (const Edge& edge : m_graph.edges()) {
        const auto sourceIt = nodesById.find(edge.sourceId);
        const auto targetIt = nodesById.find(edge.targetId);
        if (sourceIt == nodesById.end() || targetIt == nodesById.end()) {
            continue;
        }

        lineVertices.push_back(worldToClipX(sourceIt->second->x, viewport));
        lineVertices.push_back(worldToClipY(sourceIt->second->y, viewport));
        lineVertices.push_back(worldToClipX(targetIt->second->x, viewport));
        lineVertices.push_back(worldToClipY(targetIt->second->y, viewport));
    }

    return lineVertices;
}

std::vector<float> App::buildPointVertices(const Viewport& viewport) const {
    std::vector<float> pointVertices;
    pointVertices.reserve(m_graph.nodes().size() * 2U);

    for (const Node& node : m_graph.nodes()) {
        pointVertices.push_back(worldToClipX(node.x, viewport));
        pointVertices.push_back(worldToClipY(node.y, viewport));
    }

    return pointVertices;
}

float App::worldToClipX(float worldX, const Viewport& viewport) const {
    return ((worldX - viewport.centerX) / (viewport.width * 0.5f));
}

float App::worldToClipY(float worldY, const Viewport& viewport) const {
    return ((worldY - viewport.centerY) / (viewport.height * 0.5f));
}
