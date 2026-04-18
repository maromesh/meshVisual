#pragma once

struct Node {
    int id {0};
    float x {0.0f};
    float y {0.0f};
    float velocityX {0.0f};
    float velocityY {0.0f};
    float driftVelocityX {0.0f};
    float driftVelocityY {0.0f};
    bool attachedToShape {false};
    int attachedShapeId {-1};
    int attachedShapeNodeIndex {-1};
};
