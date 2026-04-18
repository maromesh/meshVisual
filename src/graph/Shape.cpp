#include "meshvisual/graph/Shape.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {
ShapeTemplate loadShapeTemplate(const std::filesystem::path& path) {
    ShapeTemplate shape;
    shape.name = path.stem().string();

    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
        const auto commentPosition = line.find('#');
        if (commentPosition != std::string::npos) {
            line.erase(commentPosition);
        }

        std::istringstream stream(line);
        ShapePoint point;
        if (stream >> point.x >> point.y) {
            shape.points.push_back(point);
        }
    }

    return shape;
}
}

bool ShapeLibrary::loadFromDirectory(const std::string& directoryPath) {
    m_templates.clear();

    const std::filesystem::path root(directoryPath);
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
        return false;
    }

    std::vector<std::filesystem::path> shapePaths;
    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        shapePaths.push_back(entry.path());
    }

    std::sort(shapePaths.begin(), shapePaths.end());
    for (const auto& shapePath : shapePaths) {
        ShapeTemplate shape = loadShapeTemplate(shapePath);
        if (!shape.points.empty()) {
            m_templates.push_back(std::move(shape));
        }
    }

    return !m_templates.empty();
}

const std::vector<ShapeTemplate>& ShapeLibrary::templates() const {
    return m_templates;
}
