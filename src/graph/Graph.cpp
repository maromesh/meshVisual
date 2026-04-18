#include "meshvisual/graph/Graph.hpp"

#include <algorithm>
#include <cmath>

void Graph::addNode(const Node& node) {
    m_nodes.push_back(node);
}

void Graph::addEdge(const Edge& edge) {
    auto existingEdgeIt = std::find_if(
        m_edges.begin(),
        m_edges.end(),
        [&edge](const Edge& existingEdge) {
            return
                (existingEdge.sourceId == edge.sourceId && existingEdge.targetId == edge.targetId) ||
                (existingEdge.sourceId == edge.targetId && existingEdge.targetId == edge.sourceId);
        }
    );
    if (existingEdgeIt != m_edges.end()) {
        existingEdgeIt->participatesInTopology = true;
        existingEdgeIt->targetAlpha = edge.targetAlpha;
        existingEdgeIt->fadeSpeed = edge.fadeSpeed;
        existingEdgeIt->alpha = std::min(existingEdgeIt->alpha, edge.alpha);
        return;
    }

    m_edges.push_back(edge);
}

void Graph::clearEdges() {
    m_edges.clear();
}

void Graph::removeEdgesForNode(int nodeId) {
    m_edges.erase(
        std::remove_if(
            m_edges.begin(),
            m_edges.end(),
            [nodeId](const Edge& edge) {
                return edge.sourceId == nodeId || edge.targetId == nodeId;
            }
        ),
        m_edges.end()
    );
}

void Graph::fadeOutEdgesForNode(int nodeId, float fadeDurationSeconds) {
    const float fadeSpeed = (fadeDurationSeconds <= 0.0f) ? 1.0f : (1.0f / fadeDurationSeconds);

    for (Edge& edge : m_edges) {
        if (edge.sourceId == nodeId || edge.targetId == nodeId) {
            edge.participatesInTopology = false;
            edge.targetAlpha = 0.0f;
            edge.fadeSpeed = fadeSpeed;
        }
    }
}

void Graph::fadeOutEdgeBetween(int firstNodeId, int secondNodeId, float fadeDurationSeconds) {
    const float fadeSpeed = (fadeDurationSeconds <= 0.0f) ? 1.0f : (1.0f / fadeDurationSeconds);

    for (Edge& edge : m_edges) {
        const bool matchesPair =
            (edge.sourceId == firstNodeId && edge.targetId == secondNodeId) ||
            (edge.sourceId == secondNodeId && edge.targetId == firstNodeId);
        if (matchesPair) {
            edge.participatesInTopology = false;
            edge.targetAlpha = 0.0f;
            edge.fadeSpeed = fadeSpeed;
        }
    }
}

void Graph::updateEdgeTransitions(float deltaSeconds) {
    for (Edge& edge : m_edges) {
        if (edge.fadeSpeed <= 0.0f || std::abs(edge.alpha - edge.targetAlpha) < 0.0001f) {
            edge.alpha = edge.targetAlpha;
            continue;
        }

        const float alphaDelta = edge.fadeSpeed * deltaSeconds;
        if (edge.alpha < edge.targetAlpha) {
            edge.alpha = std::min(edge.targetAlpha, edge.alpha + alphaDelta);
        } else {
            edge.alpha = std::max(edge.targetAlpha, edge.alpha - alphaDelta);
        }
    }

    m_edges.erase(
        std::remove_if(
            m_edges.begin(),
            m_edges.end(),
            [](const Edge& edge) {
                return !edge.participatesInTopology && edge.alpha <= 0.0001f;
            }
        ),
        m_edges.end()
    );
}

bool Graph::hasEdgeBetween(int firstNodeId, int secondNodeId) const {
    return std::any_of(
        m_edges.begin(),
        m_edges.end(),
        [firstNodeId, secondNodeId](const Edge& edge) {
            if (!edge.participatesInTopology) {
                return false;
            }

            return
                (edge.sourceId == firstNodeId && edge.targetId == secondNodeId) ||
                (edge.sourceId == secondNodeId && edge.targetId == firstNodeId);
        }
    );
}

std::size_t Graph::activeEdgeCountForNode(int nodeId) const {
    return static_cast<std::size_t>(std::count_if(
        m_edges.begin(),
        m_edges.end(),
        [nodeId](const Edge& edge) {
            return edge.participatesInTopology && (edge.sourceId == nodeId || edge.targetId == nodeId);
        }
    ));
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
