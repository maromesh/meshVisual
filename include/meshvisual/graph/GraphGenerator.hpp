#pragma once

#include "meshvisual/graph/Graph.hpp"

#include <cstddef>
#include <random>

struct GraphGenerationConfig {
    float worldWidth {3200.0f};
    float worldHeight {1800.0f};
    float pointDensity {0.00008f};
    float maxConnectionDistance {220.0f};
    std::size_t maxNeighborsPerNode {3};
    std::size_t maxEdgeChangesPerRefresh {1};
    float edgeRefreshIntervalSeconds {4.0f};
    float edgeRefreshBatchFraction {0.05f};
    float minEdgeFadeDurationSeconds {0.2f};
    float maxEdgeFadeDurationSeconds {0.8f};
    float minSpeed {10.0f};
    float maxSpeed {28.0f};
};

class GraphGenerator {
public:
    GraphGenerator();

    [[nodiscard]] Graph createRandomGraph(const GraphGenerationConfig& config) const;
    void fadeOutEdgesForNode(Graph& graph, const GraphGenerationConfig& config, int nodeId) const;
    void refreshEdgesForNode(Graph& graph, const GraphGenerationConfig& config, int nodeId) const;

private:
    [[nodiscard]] float nextFadeDuration(const GraphGenerationConfig& config) const;

private:
    mutable std::mt19937 m_randomGenerator;
};
