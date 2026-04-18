#pragma once

#include "meshvisual/graph/Edge.hpp"
#include "meshvisual/graph/Node.hpp"

#include <vector>

class Graph {
public:
    void addNode(const Node& node);
    void addEdge(const Edge& edge);
    void clearEdges();
    void removeEdgesForNode(int nodeId);
    void fadeOutEdgesForNode(int nodeId, float fadeDurationSeconds);
    void fadeOutEdgeBetween(int firstNodeId, int secondNodeId, float fadeDurationSeconds);
    void updateEdgeTransitions(float deltaSeconds);
    [[nodiscard]] bool hasEdgeBetween(int firstNodeId, int secondNodeId) const;
    [[nodiscard]] std::size_t activeEdgeCountForNode(int nodeId) const;

    [[nodiscard]] std::vector<Node>& nodes();
    [[nodiscard]] const std::vector<Node>& nodes() const;
    [[nodiscard]] std::vector<Edge>& edges();
    [[nodiscard]] const std::vector<Edge>& edges() const;

private:
    std::vector<Node> m_nodes {};
    std::vector<Edge> m_edges {};
};
