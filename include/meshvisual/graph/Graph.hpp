#pragma once

#include "meshvisual/graph/Edge.hpp"
#include "meshvisual/graph/Node.hpp"

#include <vector>

class Graph {
public:
    void addNode(const Node& node);
    void addEdge(const Edge& edge);
    void clearEdges();

    [[nodiscard]] std::vector<Node>& nodes();
    [[nodiscard]] const std::vector<Node>& nodes() const;
    [[nodiscard]] std::vector<Edge>& edges();
    [[nodiscard]] const std::vector<Edge>& edges() const;

private:
    std::vector<Node> m_nodes {};
    std::vector<Edge> m_edges {};
};
