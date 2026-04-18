#include "meshvisual/core/Window.hpp"

#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr const char* kVertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec2 aPosition;

uniform float uPointSize;

void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    gl_PointSize = uPointSize;
}
)";

constexpr const char* kFragmentShaderSource = R"(
#version 330 core

uniform vec4 uColor;

out vec4 fragColor;

void main() {
    vec2 centeredPointCoord = (gl_PointCoord * 2.0) - vec2(1.0);
    if (dot(centeredPointCoord, centeredPointCoord) > 1.0) {
        discard;
    }

    fragColor = uColor;
}
)";

struct EditorState {
    std::vector<float> points {};
    bool leftMouseWasPressed {false};
    bool enterWasPressed {false};
    bool backspaceWasPressed {false};
    bool clearWasPressed {false};
};

unsigned int compileShader(unsigned int type, const char* source) {
    const unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        int logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());

        std::cerr << "Shader compilation failed:\n" << log << '\n';
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

unsigned int createProgram() {
    const unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, kVertexShaderSource);
    if (vertexShader == 0) {
        return 0;
    }

    const unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }

    const unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        int logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());

        std::cerr << "Program link failed:\n" << log << '\n';
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

std::filesystem::path buildOutputPath(int argc, char** argv) {
    if (argc >= 2) {
        std::filesystem::path requested(argv[1]);
        if (requested.extension() != ".shape") {
            requested.replace_extension(".shape");
        }

        if (requested.has_parent_path()) {
            return requested;
        }

        return std::filesystem::path("assets/shapes") / requested;
    }

    return std::filesystem::path("assets/shapes") / "custom.shape";
}

std::vector<float> normalizedPoints(const std::vector<float>& points) {
    if (points.empty()) {
        return {};
    }

    float minX = points[0];
    float maxX = points[0];
    float minY = points[1];
    float maxY = points[1];
    for (std::size_t index = 0; index < points.size(); index += 2U) {
        minX = std::min(minX, points[index]);
        maxX = std::max(maxX, points[index]);
        minY = std::min(minY, points[index + 1U]);
        maxY = std::max(maxY, points[index + 1U]);
    }

    const float centerX = (minX + maxX) * 0.5f;
    const float centerY = (minY + maxY) * 0.5f;
    const float width = maxX - minX;
    const float height = maxY - minY;
    const float extent = std::max(width, height);
    const float scale = (extent > 0.0001f) ? (2.0f / extent) : 1.0f;

    std::vector<float> normalized;
    normalized.reserve(points.size());
    for (std::size_t index = 0; index < points.size(); index += 2U) {
        normalized.push_back((points[index] - centerX) * scale);
        normalized.push_back((points[index + 1U] - centerY) * scale);
    }

    return normalized;
}

bool saveShapeFile(const std::filesystem::path& outputPath, const std::vector<float>& points) {
    if (points.size() < 2U) {
        return false;
    }

    const std::vector<float> normalized = normalizedPoints(points);

    std::filesystem::create_directories(outputPath.parent_path());
    std::ofstream output(outputPath);
    if (!output) {
        return false;
    }

    output << "# x y\n";
    output << std::fixed << std::setprecision(6);
    for (std::size_t index = 0; index < normalized.size(); index += 2U) {
        output << normalized[index] << ' ' << normalized[index + 1U] << '\n';
    }

    return true;
}

void updateWindowTitle(GLFWwindow* window, const std::filesystem::path& outputPath, std::size_t pointCount) {
    std::ostringstream title;
    title << "shape_editor - " << outputPath.filename().string()
          << " - points: " << pointCount
          << " - click:add  backspace:undo  c:clear  enter:save  esc:quit";
    glfwSetWindowTitle(window, title.str().c_str());
}

void handleInput(Window& window, EditorState& state, const std::filesystem::path& outputPath, bool& savedThisFrame) {
    GLFWwindow* nativeWindow = window.nativeHandle();
    if (nativeWindow == nullptr) {
        return;
    }

    const int leftPressed = glfwGetMouseButton(nativeWindow, GLFW_MOUSE_BUTTON_LEFT);
    if (leftPressed == GLFW_PRESS && !state.leftMouseWasPressed) {
        double cursorX = 0.0;
        double cursorY = 0.0;
        glfwGetCursorPos(nativeWindow, &cursorX, &cursorY);

        const float width = static_cast<float>(std::max(1, window.width()));
        const float height = static_cast<float>(std::max(1, window.height()));
        const float normalizedX = static_cast<float>((cursorX / width) * 2.0 - 1.0);
        const float normalizedY = static_cast<float>(1.0 - ((cursorY / height) * 2.0));

        state.points.push_back(normalizedX);
        state.points.push_back(normalizedY);
    }
    state.leftMouseWasPressed = (leftPressed == GLFW_PRESS);

    const int backspacePressed = glfwGetKey(nativeWindow, GLFW_KEY_BACKSPACE);
    if (backspacePressed == GLFW_PRESS && !state.backspaceWasPressed && state.points.size() >= 2U) {
        state.points.pop_back();
        state.points.pop_back();
    }
    state.backspaceWasPressed = (backspacePressed == GLFW_PRESS);

    const int clearPressed = glfwGetKey(nativeWindow, GLFW_KEY_C);
    if (clearPressed == GLFW_PRESS && !state.clearWasPressed) {
        state.points.clear();
    }
    state.clearWasPressed = (clearPressed == GLFW_PRESS);

    savedThisFrame = false;
    const int enterPressed = glfwGetKey(nativeWindow, GLFW_KEY_ENTER);
    if (enterPressed == GLFW_PRESS && !state.enterWasPressed) {
        if (saveShapeFile(outputPath, state.points)) {
            savedThisFrame = true;
            std::cout << "Saved shape to " << outputPath << " with "
                      << (state.points.size() / 2U) << " points.\n";
        } else {
            std::cerr << "Failed to save shape to " << outputPath << ".\n";
        }
    }
    state.enterWasPressed = (enterPressed == GLFW_PRESS);

    if (glfwGetKey(nativeWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        window.close();
    }
}
}

int main(int argc, char** argv) {
    const std::filesystem::path outputPath = buildOutputPath(argc, argv);

    Window window;
    if (!window.create("shape_editor", 900, 900)) {
        std::cerr << "Failed to create shape editor window.\n";
        return 1;
    }

    unsigned int program = createProgram();
    if (program == 0) {
        return 1;
    }

    unsigned int vao = 0;
    unsigned int vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(2U * sizeof(float)), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::cout << "shape_editor controls:\n";
    std::cout << "  left click: add point\n";
    std::cout << "  backspace: remove last point\n";
    std::cout << "  c: clear all points\n";
    std::cout << "  enter: save to " << outputPath << '\n';
    std::cout << "  esc: quit\n";

    EditorState state;
    while (window.isOpen()) {
        window.pollEvents();

        bool savedThisFrame = false;
        handleInput(window, state, outputPath, savedThisFrame);
        updateWindowTitle(window.nativeHandle(), outputPath, state.points.size() / 2U);

        glViewport(0, 0, window.framebufferWidth(), window.framebufferHeight());
        glClearColor(0.98f, 0.98f, 0.98f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(state.points.size() * sizeof(float)),
            state.points.data(),
            GL_DYNAMIC_DRAW
        );
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glUseProgram(program);
        glBindVertexArray(vao);

        const int pointSizeLocation = glGetUniformLocation(program, "uPointSize");
        const int colorLocation = glGetUniformLocation(program, "uColor");

        if (colorLocation >= 0) {
            glUniform4f(colorLocation, 0.08f, 0.09f, 0.12f, 1.0f);
        }

        if (pointSizeLocation >= 0) {
            glUniform1f(pointSizeLocation, 10.0f);
        }
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(state.points.size() / 2U));

        if (state.points.size() >= 4U) {
            if (colorLocation >= 0) {
                glUniform4f(colorLocation, 0.20f, 0.42f, 0.85f, 0.55f);
            }
            if (pointSizeLocation >= 0) {
                glUniform1f(pointSizeLocation, 1.0f);
            }
            glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(state.points.size() / 2U));
        }

        glBindVertexArray(0);
        glUseProgram(0);

        window.swapBuffers();

        if (savedThisFrame) {
            std::cout << "The saved points are centered and scaled to fit the existing shape format.\n";
        }
    }

    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
    if (program != 0) {
        glDeleteProgram(program);
    }

    return 0;
}
