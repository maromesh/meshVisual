#include "meshvisual/graph/GraphGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace {
float distanceSquared(const Node& a, const Node& b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return (dx * dx) + (dy * dy);
}

void populateEdges(Graph& graph, const GraphGenerationConfig& config) {
    graph.clearEdges();

    const float maxDistanceSquared = config.maxConnectionDistance * config.maxConnectionDistance;
    const std::vector<Node>& nodes = graph.nodes();

    for (std::size_t sourceIndex = 0; sourceIndex < nodes.size(); ++sourceIndex) {
        std::vector<std::pair<float, int>> candidateTargets;
        candidateTargets.reserve(nodes.size());

        for (std::size_t targetIndex = sourceIndex + 1; targetIndex < nodes.size(); ++targetIndex) {
            const float candidateDistanceSquared = distanceSquared(nodes[sourceIndex], nodes[targetIndex]);
            if (candidateDistanceSquared > maxDistanceSquared) {
                continue;
            }

            candidateTargets.emplace_back(candidateDistanceSquared, static_cast<int>(targetIndex));
        }

        std::sort(
            candidateTargets.begin(),
            candidateTargets.end(),
            [](const auto& left, const auto& right) {
                return left.first < right.first;
            }
        );

        const std::size_t edgeCount = std::min(config.maxNeighborsPerNode, candidateTargets.size());
        for (std::size_t edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex) {
            graph.addEdge({
                .sourceId = nodes[sourceIndex].id,
                .targetId = nodes[static_cast<std::size_t>(candidateTargets[edgeIndex].second)].id,
            });
        }
    }
}
}

Graph GraphGenerator::createRandomGraph(const GraphGenerationConfig& config) const {
    Graph graph;

    const float worldArea = std::max(1.0f, config.worldWidth * config.worldHeight);
    const std::size_t nodeCount = std::max<std::size_t>(
        2,
        static_cast<std::size_t>(std::lround(worldArea * config.pointDensity))
    );

    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_real_distribution<float> xDistribution(0.0f, config.worldWidth);
    std::uniform_real_distribution<float> yDistribution(0.0f, config.worldHeight);
    std::uniform_real_distribution<float> angleDistribution(0.0f, 6.28318530718f);
    std::uniform_real_distribution<float> speedDistribution(config.minSpeed, config.maxSpeed);

    for (std::size_t index = 0; index < nodeCount; ++index) {
        const float angle = angleDistribution(generator);
        const float speed = speedDistribution(generator);

        graph.addNode({
            .id = static_cast<int>(index),
            .x = xDistribution(generator),
            .y = yDistribution(generator),
            .velocityX = std::cos(angle) * speed,
            .velocityY = std::sin(angle) * speed,
        });
    }

    populateEdges(graph, config);

    return graph;
}

void GraphGenerator::rebuildEdges(Graph& graph, const GraphGenerationConfig& config) const {
    populateEdges(graph, config);
}
