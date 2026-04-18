#include "meshvisual/core/Settings.hpp"

#include <fstream>
#include <iomanip>
#include <optional>
#include <regex>
#include <sstream>
#include <string>

namespace {
std::string readTextFile(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::optional<float> parseFloatValue(const std::string& content, const std::string& key) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (!std::regex_search(content, match, pattern)) {
        return std::nullopt;
    }

    return std::stof(match[1].str());
}

std::optional<std::size_t> parseSizeValue(const std::string& content, const std::string& key) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*([0-9]+)");
    std::smatch match;
    if (!std::regex_search(content, match, pattern)) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::stoull(match[1].str()));
}

std::optional<int> parseIntValue(const std::string& content, const std::string& key) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+)");
    std::smatch match;
    if (!std::regex_search(content, match, pattern)) {
        return std::nullopt;
    }

    return std::stoi(match[1].str());
}

std::optional<std::string> parseStringValue(const std::string& content, const std::string& key) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    if (!std::regex_search(content, match, pattern)) {
        return std::nullopt;
    }

    return match[1].str();
}

std::optional<ColorRgba> parseHexColor(const std::string& value) {
    if (value.size() != 7U && value.size() != 9U) {
        return std::nullopt;
    }
    if (value[0] != '#') {
        return std::nullopt;
    }

    auto parseChannel = [&value](std::size_t offset) -> std::optional<float> {
        unsigned int channel = 0;
        std::istringstream stream(value.substr(offset, 2));
        stream >> std::hex >> channel;
        if (stream.fail()) {
            return std::nullopt;
        }

        return static_cast<float>(channel) / 255.0f;
    };

    const auto red = parseChannel(1);
    const auto green = parseChannel(3);
    const auto blue = parseChannel(5);
    if (!red || !green || !blue) {
        return std::nullopt;
    }

    float alpha = 1.0f;
    if (value.size() == 9U) {
        const auto parsedAlpha = parseChannel(7);
        if (!parsedAlpha) {
            return std::nullopt;
        }
        alpha = *parsedAlpha;
    }

    return ColorRgba{*red, *green, *blue, alpha};
}
}

AppSettings Settings::loadAppSettings(const std::string& path) {
    AppSettings settings;
    GraphGenerationConfig& config = settings.graph;
    const std::string content = readTextFile(path);
    if (content.empty()) {
        return settings;
    }

    if (const auto value = parseIntValue(content, "windowWidth")) {
        settings.windowWidth = *value;
    }
    if (const auto value = parseIntValue(content, "windowHeight")) {
        settings.windowHeight = *value;
    }
    if (const auto value = parseFloatValue(content, "pointSize")) {
        settings.pointSize = *value;
    }
    if (const auto value = parseFloatValue(content, "lineWidth")) {
        settings.lineWidth = *value;
    }
    if (const auto value = parseStringValue(content, "backgroundColor")) {
        if (const auto color = parseHexColor(*value)) {
            settings.backgroundColor = *color;
        }
    }
    if (const auto value = parseStringValue(content, "pointColor")) {
        if (const auto color = parseHexColor(*value)) {
            settings.pointColor = *color;
        }
    }
    if (const auto value = parseStringValue(content, "edgeColor")) {
        if (const auto color = parseHexColor(*value)) {
            settings.edgeColor = *color;
        }
    }
    if (const auto value = parseFloatValue(content, "worldWidth")) {
        config.worldWidth = *value;
    }
    if (const auto value = parseFloatValue(content, "worldHeight")) {
        config.worldHeight = *value;
    }
    if (const auto value = parseFloatValue(content, "pointDensity")) {
        config.pointDensity = *value;
    }
    if (const auto value = parseFloatValue(content, "maxConnectionDistance")) {
        config.maxConnectionDistance = *value;
    }
    if (const auto value = parseSizeValue(content, "maxNeighborsPerNode")) {
        config.maxNeighborsPerNode = *value;
    }
    if (const auto value = parseSizeValue(content, "maxEdgeChangesPerRefresh")) {
        config.maxEdgeChangesPerRefresh = *value;
    }
    if (const auto value = parseFloatValue(content, "edgeRefreshIntervalSeconds")) {
        config.edgeRefreshIntervalSeconds = *value;
    }
    if (const auto value = parseFloatValue(content, "edgeRefreshBatchFraction")) {
        config.edgeRefreshBatchFraction = *value;
    }
    if (const auto value = parseFloatValue(content, "minEdgeFadeDurationSeconds")) {
        config.minEdgeFadeDurationSeconds = *value;
    }
    if (const auto value = parseFloatValue(content, "maxEdgeFadeDurationSeconds")) {
        config.maxEdgeFadeDurationSeconds = *value;
    }
    if (const auto value = parseFloatValue(content, "minSpeed")) {
        config.minSpeed = *value;
    }
    if (const auto value = parseFloatValue(content, "maxSpeed")) {
        config.maxSpeed = *value;
    }

    return settings;
}
