#include "meshvisual/graph/Graph.hpp"

void Graph::addNode(const Node& node) {
    m_nodes.push_back(node);
}

void Graph::addEdge(const Edge& edge) {
    m_edges.push_back(edge);
}

void Graph::clearEdges() {
    m_edges.clear();
}

std::vector<Node>& Graph::nodes() {
    return m_nodes;
}

const std::vector<Node>& Graph::nodes() const {
    return m_nodes;
}

std::vector<Edge>& Graph::edges() {
    return m_edges;
}

const std::vector<Edge>& Graph::edges() const {
    return m_edges;
}
