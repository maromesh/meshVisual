#pragma once

#include <string>
#include <vector>

struct ShapePoint {
    float x {0.0f};
    float y {0.0f};
};

struct ShapeTemplate {
    std::string name;
    std::vector<ShapePoint> points;
};

struct ActiveShapePoint {
    float localX {0.0f};
    float localY {0.0f};
    int attachedNodeId {-1};
    bool attractionEnabled {true};
};

struct ActiveShape {
    int id {0};
    std::string name;
    float centerX {0.0f};
    float centerY {0.0f};
    float velocityX {0.0f};
    float velocityY {0.0f};
    float remainingLifetimeSeconds {0.0f};
    float captureRadius {0.0f};
    float attractionStrength {0.0f};
    float minLocalX {0.0f};
    float maxLocalX {0.0f};
    float minLocalY {0.0f};
    float maxLocalY {0.0f};
    std::vector<ActiveShapePoint> points;
};

class ShapeLibrary {
public:
    [[nodiscard]] bool loadFromDirectory(const std::string& directoryPath);
    [[nodiscard]] const std::vector<ShapeTemplate>& templates() const;

private:
    std::vector<ShapeTemplate> m_templates {};
};
