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
                .alpha = 1.0f,
                .targetAlpha = 1.0f,
                .fadeSpeed = 0.0f,
                .participatesInTopology = true,
            });
        }
    }
}

const Node* findNodeById(const Graph& graph, int nodeId) {
    const std::vector<Node>& nodes = graph.nodes();
    const auto nodeIt = std::find_if(
        nodes.begin(),
        nodes.end(),
        [nodeId](const Node& node) {
            return node.id == nodeId;
        }
    );
    if (nodeIt == nodes.end()) {
        return nullptr;
    }

    return &(*nodeIt);
}
}

GraphGenerator::GraphGenerator()
    : m_randomGenerator(std::random_device{}()) {}

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

void GraphGenerator::fadeOutEdgesForNode(Graph& graph, const GraphGenerationConfig& config, int nodeId) const {
    std::vector<int> neighborIds;
    neighborIds.reserve(graph.edges().size());

    for (const Edge& edge : graph.edges()) {
        if (!edge.participatesInTopology) {
            continue;
        }

        if (edge.sourceId == nodeId) {
            neighborIds.push_back(edge.targetId);
        } else if (edge.targetId == nodeId) {
            neighborIds.push_back(edge.sourceId);
        }
    }

    for (const int neighborId : neighborIds) {
        graph.fadeOutEdgeBetween(nodeId, neighborId, nextFadeDuration(config));
    }
}

void GraphGenerator::refreshEdgesForNode(Graph& graph, const GraphGenerationConfig& config, int nodeId) const {
    const Node* source = findNodeById(graph, nodeId);
    if (source == nullptr) {
        return;
    }

    const std::size_t changeBudget = config.maxEdgeChangesPerRefresh;
    if (changeBudget == 0U) {
        return;
    }

    const float maxDistanceSquared = config.maxConnectionDistance * config.maxConnectionDistance;

    std::vector<std::pair<float, int>> currentNeighbors;
    currentNeighbors.reserve(graph.edges().size());

    for (const Edge& edge : graph.edges()) {
        int neighborId = -1;
        if (edge.sourceId == nodeId) {
            neighborId = edge.targetId;
        } else if (edge.targetId == nodeId) {
            neighborId = edge.sourceId;
        } else {
            continue;
        }

        const Node* neighbor = findNodeById(graph, neighborId);
        if (neighbor == nullptr) {
            continue;
        }

        currentNeighbors.emplace_back(distanceSquared(*source, *neighbor), neighborId);
    }

    std::sort(
        currentNeighbors.begin(),
        currentNeighbors.end(),
        [](const auto& left, const auto& right) {
            return left.first < right.first;
        }
    );

    std::vector<std::pair<float, int>> candidateTargets;
    candidateTargets.reserve(graph.nodes().size());

    for (const Node& candidate : graph.nodes()) {
        if (candidate.id == nodeId || graph.hasEdgeBetween(nodeId, candidate.id)) {
            continue;
        }

        const float candidateDistanceSquared = distanceSquared(*source, candidate);
        if (candidateDistanceSquared > maxDistanceSquared) {
            continue;
        }

        candidateTargets.emplace_back(candidateDistanceSquared, candidate.id);
    }

    std::sort(
        candidateTargets.begin(),
        candidateTargets.end(),
        [](const auto& left, const auto& right) {
            return left.first < right.first;
        }
    );

    std::size_t removedCount = 0U;
    std::size_t candidateIndex = 0U;

    while (removedCount < changeBudget) {
        auto invalidEdgeIt = std::find_if(
            currentNeighbors.rbegin(),
            currentNeighbors.rend(),
            [maxDistanceSquared](const auto& entry) {
                return entry.first > maxDistanceSquared;
            }
        );
        if (invalidEdgeIt != currentNeighbors.rend()) {
            graph.fadeOutEdgeBetween(nodeId, invalidEdgeIt->second, nextFadeDuration(config));
            const int neighborId = invalidEdgeIt->second;
            currentNeighbors.erase(
                std::remove_if(
                    currentNeighbors.begin(),
                    currentNeighbors.end(),
                    [neighborId](const auto& entry) {
                        return entry.second == neighborId;
                    }
                ),
                currentNeighbors.end()
            );
            ++removedCount;
            continue;
        }

        const std::size_t currentEdgeCount = graph.activeEdgeCountForNode(nodeId);
        while (candidateIndex < candidateTargets.size() && graph.hasEdgeBetween(nodeId, candidateTargets[candidateIndex].second)) {
            ++candidateIndex;
        }
        if (currentEdgeCount >= config.maxNeighborsPerNode &&
            !currentNeighbors.empty() &&
            candidateIndex < candidateTargets.size()) {
            const auto& farthestCurrent = currentNeighbors.back();
            const auto& nearestCandidate = candidateTargets[candidateIndex];
            if (nearestCandidate.first < farthestCurrent.first) {
                graph.fadeOutEdgeBetween(nodeId, farthestCurrent.second, nextFadeDuration(config));
                currentNeighbors.pop_back();
                ++removedCount;
                continue;
            }
        }

        break;
    }

    std::size_t addedCount = 0U;
    while (addedCount < changeBudget) {
        const std::size_t refreshedEdgeCount = graph.activeEdgeCountForNode(nodeId);
        if (refreshedEdgeCount >= config.maxNeighborsPerNode) {
            break;
        }

        while (candidateIndex < candidateTargets.size() && graph.hasEdgeBetween(nodeId, candidateTargets[candidateIndex].second)) {
            ++candidateIndex;
        }
        if (candidateIndex >= candidateTargets.size()) {
            break;
        }

        const float fadeDuration = nextFadeDuration(config);
        graph.addEdge({
            .sourceId = nodeId,
            .targetId = candidateTargets[candidateIndex].second,
            .alpha = 0.0f,
            .targetAlpha = 1.0f,
            .fadeSpeed = (fadeDuration <= 0.0f) ? 1.0f : (1.0f / fadeDuration),
            .participatesInTopology = true,
        });
        ++candidateIndex;
        ++addedCount;
    }
}

float GraphGenerator::nextFadeDuration(const GraphGenerationConfig& config) const {
    const float minDuration = std::max(0.01f, config.minEdgeFadeDurationSeconds);
    const float maxDuration = std::max(minDuration, config.maxEdgeFadeDurationSeconds);
    std::uniform_real_distribution<float> distribution(minDuration, maxDuration);
    return distribution(m_randomGenerator);
}
