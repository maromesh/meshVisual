#pragma once

struct Edge {
    int sourceId {0};
    int targetId {0};
    float alpha {1.0f};
    float targetAlpha {1.0f};
    float fadeSpeed {0.0f};
    bool participatesInTopology {true};
};
