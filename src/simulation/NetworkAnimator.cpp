#include "meshvisual/simulation/NetworkAnimator.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kTwoPi = 6.28318530718f;

float lengthSquared(float x, float y) {
    return (x * x) + (y * y);
}
}

NetworkAnimator::NetworkAnimator()
    : m_randomGenerator(std::random_device{}()) {}

bool NetworkAnimator::loadShapes(const std::string& directoryPath) {
    const bool loaded = m_shapeLibrary.loadFromDirectory(directoryPath);
    m_spawnCountdownSeconds = 0.0f;
    return loaded;
}

void NetworkAnimator::update(
    Graph& graph,
    const GraphGenerationConfig& config,
    float viewportWidth,
    float viewportHeight,
    float deltaSeconds
) {
    if (deltaSeconds <= 0.0f) {
        return;
    }

    updateSpawnTimer(graph, config, viewportWidth, viewportHeight, deltaSeconds);
    updateActiveShapes(graph, config, deltaSeconds);
    releaseExpiredShapes(graph);
}

void NetworkAnimator::updateSpawnTimer(
    Graph& graph,
    const GraphGenerationConfig& config,
    float viewportWidth,
    float viewportHeight,
    float deltaSeconds
) {
    if (m_shapeLibrary.templates().empty() || config.maxConcurrentShapes == 0U) {
        return;
    }

    m_spawnCountdownSeconds -= deltaSeconds;
    if (m_spawnCountdownSeconds > 0.0f) {
        return;
    }

    if (m_activeShapes.size() < config.maxConcurrentShapes) {
        spawnShape(graph, config, viewportWidth, viewportHeight);
    }
    m_spawnCountdownSeconds = nextSpawnIntervalSeconds(config);
}

void NetworkAnimator::spawnShape(
    Graph& graph,
    const GraphGenerationConfig& config,
    float viewportWidth,
    float viewportHeight
) {
    const auto& templates = m_shapeLibrary.templates();
    if (templates.empty()) {
        return;
    }

    const ShapeTemplate& shapeTemplate = templates[static_cast<std::size_t>(
        randomInt(0, static_cast<int>(templates.size()) - 1)
    )];
    if (shapeTemplate.points.empty()) {
        return;
    }

    float minX = shapeTemplate.points.front().x;
    float maxX = shapeTemplate.points.front().x;
    float minY = shapeTemplate.points.front().y;
    float maxY = shapeTemplate.points.front().y;
    for (const ShapePoint& point : shapeTemplate.points) {
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }

    const float sourceWidth = std::max(0.001f, maxX - minX);
    const float sourceHeight = std::max(0.001f, maxY - minY);
    const float sourceArea = sourceWidth * sourceHeight;
    const float viewportArea = std::max(1.0f, viewportWidth * viewportHeight);
    const float minViewportArea = std::max(0.0001f, config.minShapeViewportAreaFraction);
    const float maxViewportArea = std::max(minViewportArea, config.maxShapeViewportAreaFraction);
    const float targetArea = viewportArea * randomFloat(minViewportArea, maxViewportArea);
    const float scale = std::sqrt(targetArea / sourceArea);

    const float centeredOffsetX = (minX + maxX) * 0.5f;
    const float centeredOffsetY = (minY + maxY) * 0.5f;
    const float angle = randomFloat(0.0f, kTwoPi);
    const float speed = randomFloat(config.minShapeSpeed, config.maxShapeSpeed);

    ActiveShape shape;
    shape.id = m_nextShapeId++;
    shape.name = shapeTemplate.name;
    shape.velocityX = std::cos(angle) * speed;
    shape.velocityY = std::sin(angle) * speed;
    shape.remainingLifetimeSeconds = randomFloat(
        config.minShapeLifetimeSeconds,
        config.maxShapeLifetimeSeconds
    );
    shape.captureRadius = config.shapeCaptureRadius;
    shape.attractionStrength = config.shapeAttractionStrength;
    shape.points.reserve(shapeTemplate.points.size());
    shape.minLocalX = 0.0f;
    shape.maxLocalX = 0.0f;
    shape.minLocalY = 0.0f;
    shape.maxLocalY = 0.0f;

    bool firstPoint = true;
    for (const ShapePoint& templatePoint : shapeTemplate.points) {
        const float localX = (templatePoint.x - centeredOffsetX) * scale;
        const float localY = (templatePoint.y - centeredOffsetY) * scale;
        shape.points.push_back({
            .localX = localX,
            .localY = localY,
            .attachedNodeId = -1,
            .attractionEnabled = true,
        });

        if (firstPoint) {
            shape.minLocalX = localX;
            shape.maxLocalX = localX;
            shape.minLocalY = localY;
            shape.maxLocalY = localY;
            firstPoint = false;
            continue;
        }

        shape.minLocalX = std::min(shape.minLocalX, localX);
        shape.maxLocalX = std::max(shape.maxLocalX, localX);
        shape.minLocalY = std::min(shape.minLocalY, localY);
        shape.maxLocalY = std::max(shape.maxLocalY, localY);
    }

    const float minCenterX = std::max(0.0f, -shape.minLocalX);
    const float maxCenterX = std::max(minCenterX, config.worldWidth - shape.maxLocalX);
    const float minCenterY = std::max(0.0f, -shape.minLocalY);
    const float maxCenterY = std::max(minCenterY, config.worldHeight - shape.maxLocalY);
    shape.centerX = randomFloat(minCenterX, maxCenterX);
    shape.centerY = randomFloat(minCenterY, maxCenterY);

    assignNodesToShape(graph, shape, config);
    m_activeShapes.push_back(std::move(shape));
}

void NetworkAnimator::updateActiveShapes(Graph& graph, const GraphGenerationConfig& config, float deltaSeconds) {
    for (ActiveShape& shape : m_activeShapes) {
        shape.remainingLifetimeSeconds -= deltaSeconds;
        updateShapeMotion(shape, config, deltaSeconds);

        for (ActiveShapePoint& point : shape.points) {
            if (point.attachedNodeId < 0) {
                continue;
            }

            auto nodeIt = std::find_if(
                graph.nodes().begin(),
                graph.nodes().end(),
                [&point](const Node& node) {
                    return node.id == point.attachedNodeId;
                }
            );
            if (nodeIt == graph.nodes().end()) {
                point.attachedNodeId = -1;
                point.attractionEnabled = true;
                continue;
            }

            const ShapePoint worldPoint = worldPointForShapeNode(shape, point, config);
            const float dx = worldPoint.x - nodeIt->x;
            const float dy = worldPoint.y - nodeIt->y;
            const float distanceSquared = lengthSquared(dx, dy);
            const float captureRadiusSquared = shape.captureRadius * shape.captureRadius;

            if (distanceSquared <= captureRadiusSquared) {
                nodeIt->x = worldPoint.x;
                nodeIt->y = worldPoint.y;
                nodeIt->velocityX = shape.velocityX;
                nodeIt->velocityY = shape.velocityY;
                continue;
            }

            if (distanceSquared <= 0.0001f) {
                continue;
            }

            const float distance = std::sqrt(distanceSquared);
            const float directionX = dx / distance;
            const float directionY = dy / distance;
            const float availableDistance = std::max(0.0f, distance - shape.captureRadius);
            const float maxApproachStep = std::max(0.0f, shape.attractionStrength * deltaSeconds);
            const float appliedStep = std::min(availableDistance, maxApproachStep);

            nodeIt->velocityX = shape.velocityX + (directionX * (appliedStep / std::max(deltaSeconds, 0.0001f)));
            nodeIt->velocityY = shape.velocityY + (directionY * (appliedStep / std::max(deltaSeconds, 0.0001f)));
            nodeIt->x += (shape.velocityX * deltaSeconds) + (directionX * appliedStep);
            nodeIt->y += (shape.velocityY * deltaSeconds) + (directionY * appliedStep);
        }
    }
}

void NetworkAnimator::updateShapeMotion(ActiveShape& shape, const GraphGenerationConfig& config, float deltaSeconds) const {
    float nextCenterX = shape.centerX + (shape.velocityX * deltaSeconds);
    float nextCenterY = shape.centerY + (shape.velocityY * deltaSeconds);

    if ((nextCenterX + shape.minLocalX) < 0.0f || (nextCenterX + shape.maxLocalX) > config.worldWidth) {
        shape.velocityX = -shape.velocityX;
        nextCenterX = shape.centerX + (shape.velocityX * deltaSeconds);
    }
    if ((nextCenterY + shape.minLocalY) < 0.0f || (nextCenterY + shape.maxLocalY) > config.worldHeight) {
        shape.velocityY = -shape.velocityY;
        nextCenterY = shape.centerY + (shape.velocityY * deltaSeconds);
    }

    shape.centerX = std::clamp(nextCenterX, -shape.minLocalX, config.worldWidth - shape.maxLocalX);
    shape.centerY = std::clamp(nextCenterY, -shape.minLocalY, config.worldHeight - shape.maxLocalY);
}

void NetworkAnimator::assignNodesToShape(Graph& graph, ActiveShape& shape, const GraphGenerationConfig& config) const {
    std::vector<Node*> availableNodes;
    availableNodes.reserve(graph.nodes().size());

    for (Node& node : graph.nodes()) {
        if (!node.attachedToShape) {
            availableNodes.push_back(&node);
        }
    }

    std::sort(
        availableNodes.begin(),
        availableNodes.end(),
        [&shape](const Node* left, const Node* right) {
            const float leftDistance = lengthSquared(left->x - shape.centerX, left->y - shape.centerY);
            const float rightDistance = lengthSquared(right->x - shape.centerX, right->y - shape.centerY);
            return leftDistance < rightDistance;
        }
    );

    const std::size_t assignmentCount = std::min(shape.points.size(), availableNodes.size());
    std::vector<bool> pointAssigned(shape.points.size(), false);

    for (std::size_t nodeIndex = 0; nodeIndex < assignmentCount; ++nodeIndex) {
        Node& node = *availableNodes[nodeIndex];
        std::size_t selectedPointIndex = shape.points.size();
        float bestDistanceSquared = 0.0f;

        for (std::size_t pointIndex = 0; pointIndex < shape.points.size(); ++pointIndex) {
            if (pointAssigned[pointIndex]) {
                continue;
            }

            const ShapePoint targetPoint = worldPointForShapeNode(shape, shape.points[pointIndex], config);
            const float distanceSquared = lengthSquared(node.x - targetPoint.x, node.y - targetPoint.y);
            if (selectedPointIndex == shape.points.size() || distanceSquared < bestDistanceSquared) {
                selectedPointIndex = pointIndex;
                bestDistanceSquared = distanceSquared;
            }
        }

        if (selectedPointIndex == shape.points.size()) {
            break;
        }

        pointAssigned[selectedPointIndex] = true;
        ActiveShapePoint& point = shape.points[selectedPointIndex];
        point.attachedNodeId = node.id;
        point.attractionEnabled = false;
        node.attachedToShape = true;
        node.attachedShapeId = shape.id;
        node.attachedShapeNodeIndex = static_cast<int>(selectedPointIndex);
    }
}

void NetworkAnimator::releaseExpiredShapes(Graph& graph) {
    for (ActiveShape& shape : m_activeShapes) {
        if (shape.remainingLifetimeSeconds > 0.0f) {
            continue;
        }

        for (const ActiveShapePoint& point : shape.points) {
            if (point.attachedNodeId < 0) {
                continue;
            }

            auto nodeIt = std::find_if(
                graph.nodes().begin(),
                graph.nodes().end(),
                [&point](const Node& node) {
                    return node.id == point.attachedNodeId;
                }
            );
            if (nodeIt != graph.nodes().end()) {
                releaseNode(*nodeIt);
            }
        }
    }

    m_activeShapes.erase(
        std::remove_if(
            m_activeShapes.begin(),
            m_activeShapes.end(),
            [](const ActiveShape& shape) {
                return shape.remainingLifetimeSeconds <= 0.0f;
            }
        ),
        m_activeShapes.end()
    );
}

void NetworkAnimator::releaseNode(Node& node) const {
    node.attachedToShape = false;
    node.attachedShapeId = -1;
    node.attachedShapeNodeIndex = -1;
    node.velocityX = node.driftVelocityX;
    node.velocityY = node.driftVelocityY;
}

float NetworkAnimator::randomFloat(float minValue, float maxValue) {
    const float clampedMax = std::max(minValue, maxValue);
    std::uniform_real_distribution<float> distribution(minValue, clampedMax);
    return distribution(m_randomGenerator);
}

int NetworkAnimator::randomInt(int minValue, int maxValue) {
    std::uniform_int_distribution<int> distribution(minValue, std::max(minValue, maxValue));
    return distribution(m_randomGenerator);
}

ShapePoint NetworkAnimator::worldPointForShapeNode(
    const ActiveShape& shape,
    const ActiveShapePoint& point,
    const GraphGenerationConfig& config
) const {
    (void)config;
    return ShapePoint {
        shape.centerX + point.localX,
        shape.centerY + point.localY,
    };
}

float NetworkAnimator::nextSpawnIntervalSeconds(const GraphGenerationConfig& config) {
    return randomFloat(config.minShapeSpawnIntervalSeconds, config.maxShapeSpawnIntervalSeconds);
}
