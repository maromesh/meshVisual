#include "meshvisual/App.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <numeric>
#include <unordered_map>
#include <vector>

int App::run() {
    m_settings = Settings::loadAppSettings("config/settings.json");
    m_generationConfig = m_settings.graph;

    const bool windowCreated = m_window.create("meshVisual", m_settings.windowWidth, m_settings.windowHeight);
    if (!windowCreated) {
        return 1;
    }

    const bool rendererInitialized = m_renderer.initialize();
    if (!rendererInitialized) {
        return 1;
    }

    m_renderer.configure(m_settings);
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
        m_renderer.clear(
            m_settings.backgroundColor.red,
            m_settings.backgroundColor.green,
            m_settings.backgroundColor.blue,
            m_settings.backgroundColor.alpha
        );
        m_renderer.drawLines();
        m_renderer.drawPoints();

        m_window.swapBuffers();
    }

    m_window.destroy();
    return 0;
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

    m_edgeRefreshAccumulator += deltaSeconds;

    for (Node& node : m_graph.nodes()) {
        bool wrapped = false;

        node.x += node.velocityX * deltaSeconds;
        node.y += node.velocityY * deltaSeconds;

        if (node.x < 0.0f) {
            node.x += m_generationConfig.worldWidth;
            wrapped = true;
        } else if (node.x > m_generationConfig.worldWidth) {
            node.x -= m_generationConfig.worldWidth;
            wrapped = true;
        }

        if (node.y < 0.0f) {
            node.y += m_generationConfig.worldHeight;
            wrapped = true;
        } else if (node.y > m_generationConfig.worldHeight) {
            node.y -= m_generationConfig.worldHeight;
            wrapped = true;
        }

        if (wrapped) {
            m_graph.removeEdgesForNode(node.id);
            m_graphGenerator.refreshEdgesForNode(m_graph, m_generationConfig, node.id);
        }
    }

    if (m_edgeRefreshAccumulator >= std::max(0.1f, m_generationConfig.edgeRefreshIntervalSeconds)) {
        m_edgeRefreshAccumulator = 0.0f;
        refreshRandomNodeSubset();
    }

    m_graph.updateEdgeTransitions(deltaSeconds);
}

void App::refreshRandomNodeSubset() {
    const std::size_t batchSize = nodeRefreshBatchSize();
    if (batchSize == 0U) {
        return;
    }

    std::vector<int> nodeIds;
    nodeIds.reserve(m_graph.nodes().size());
    for (const Node& node : m_graph.nodes()) {
        nodeIds.push_back(node.id);
    }

    std::shuffle(nodeIds.begin(), nodeIds.end(), m_randomGenerator);
    const std::size_t refreshCount = std::min(batchSize, nodeIds.size());

    for (std::size_t index = 0; index < refreshCount; ++index) {
        m_graphGenerator.refreshEdgesForNode(m_graph, m_generationConfig, nodeIds[index]);
    }
}

std::size_t App::nodeRefreshBatchSize() const {
    const std::size_t nodeCount = m_graph.nodes().size();
    if (nodeCount == 0U) {
        return 0U;
    }

    const float clampedFraction = std::clamp(m_generationConfig.edgeRefreshBatchFraction, 0.0f, 1.0f);
    const std::size_t batchSize = static_cast<std::size_t>(std::ceil(static_cast<float>(nodeCount) * clampedFraction));
    return std::max<std::size_t>(1U, batchSize);
}

std::vector<float> App::buildLineVertices(const Viewport& viewport) const {
    std::unordered_map<int, const Node*> nodesById;
    nodesById.reserve(m_graph.nodes().size());

    for (const Node& node : m_graph.nodes()) {
        nodesById.emplace(node.id, &node);
    }

    std::vector<float> lineVertices;
    lineVertices.reserve(m_graph.edges().size() * 6U);

    for (const Edge& edge : m_graph.edges()) {
        if (edge.alpha <= 0.0001f) {
            continue;
        }

        const auto sourceIt = nodesById.find(edge.sourceId);
        const auto targetIt = nodesById.find(edge.targetId);
        if (sourceIt == nodesById.end() || targetIt == nodesById.end()) {
            continue;
        }

        lineVertices.push_back(worldToClipX(sourceIt->second->x, viewport));
        lineVertices.push_back(worldToClipY(sourceIt->second->y, viewport));
        lineVertices.push_back(edge.alpha);
        lineVertices.push_back(worldToClipX(targetIt->second->x, viewport));
        lineVertices.push_back(worldToClipY(targetIt->second->y, viewport));
        lineVertices.push_back(edge.alpha);
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
