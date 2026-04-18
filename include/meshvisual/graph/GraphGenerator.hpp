#pragma once

#include "meshvisual/graph/Graph.hpp"

#include <cstddef>

struct GraphGenerationConfig {
    float worldWidth {3200.0f};
    float worldHeight {1800.0f};
    float pointDensity {0.00008f};
    float maxConnectionDistance {220.0f};
    std::size_t maxNeighborsPerNode {3};
    float minSpeed {10.0f};
    float maxSpeed {28.0f};
};

class GraphGenerator {
public:
    [[nodiscard]] Graph createRandomGraph(const GraphGenerationConfig& config) const;
    void rebuildEdges(Graph& graph, const GraphGenerationConfig& config) const;
};
