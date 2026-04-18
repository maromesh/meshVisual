#pragma once

#include "meshvisual/graph/Graph.hpp"
#include "meshvisual/graph/GraphGenerator.hpp"
#include "meshvisual/graph/Shape.hpp"

#include <random>
#include <string>
#include <vector>

class NetworkAnimator {
public:
    NetworkAnimator();

    bool loadShapes(const std::string& directoryPath);
    void update(Graph& graph, const GraphGenerationConfig& config, float viewportWidth, float viewportHeight, float deltaSeconds);

private:
    void updateSpawnTimer(Graph& graph, const GraphGenerationConfig& config, float viewportWidth, float viewportHeight, float deltaSeconds);
    void spawnShape(Graph& graph, const GraphGenerationConfig& config, float viewportWidth, float viewportHeight);
    void updateActiveShapes(Graph& graph, const GraphGenerationConfig& config, float deltaSeconds);
    void updateShapeMotion(ActiveShape& shape, const GraphGenerationConfig& config, float deltaSeconds) const;
    void assignNodesToShape(Graph& graph, ActiveShape& shape, const GraphGenerationConfig& config) const;
    void releaseExpiredShapes(Graph& graph);
    void releaseNode(Node& node) const;
    [[nodiscard]] float randomFloat(float minValue, float maxValue);
    [[nodiscard]] int randomInt(int minValue, int maxValue);
    [[nodiscard]] ShapePoint worldPointForShapeNode(const ActiveShape& shape, const ActiveShapePoint& point, const GraphGenerationConfig& config) const;
    [[nodiscard]] float nextSpawnIntervalSeconds(const GraphGenerationConfig& config);

private:
    ShapeLibrary m_shapeLibrary;
    std::vector<ActiveShape> m_activeShapes {};
    mutable std::mt19937 m_randomGenerator;
    float m_spawnCountdownSeconds {0.0f};
    int m_nextShapeId {1};
};
