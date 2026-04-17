#include "meshvisual/core/Renderer.hpp"

#include <OpenGL/gl3.h>

#include <array>
#include <iostream>
#include <string>

namespace {
constexpr const char* kPointVertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec2 aPosition;

void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    gl_PointSize = 10.0;
}
)";

constexpr const char* kPointFragmentShaderSource = R"(
#version 330 core

out vec4 fragColor;

void main() {
    fragColor = vec4(0.90, 0.95, 1.00, 1.0);
}
)";
}

Renderer::~Renderer() {
    destroy();
}

bool Renderer::initialize() {
    if (m_initialized) {
        return true;
    }

    glEnable(GL_PROGRAM_POINT_SIZE);

    if (!createPointPipeline()) {
        destroy();
        return false;
    }

    m_initialized = true;
    return true;
}

void Renderer::beginFrame(int width, int height) {
    glViewport(0, 0, width, height);
}

void Renderer::clear(float red, float green, float blue, float alpha) {
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::drawPoints() {
    if (!m_initialized || m_pointProgram == 0 || m_pointVao == 0) {
        return;
    }

    glUseProgram(m_pointProgram);
    glBindVertexArray(m_pointVao);
    glDrawArrays(GL_POINTS, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::destroy() {
    if (m_pointVbo != 0) {
        glDeleteProgram(m_pointVbo);
        m_pointVbo = 0;
    }

    if (m_pointVao != 0) {
        glDeleteProgram(m_pointVao);
        m_pointVao = 0;
    }
    
    if (m_pointProgram != 0) {
        glDeleteProgram(m_pointProgram);
        m_pointProgram = 0;
    }
    m_initialized = false;
}

unsigned int Renderer::compileShader(unsigned int type, const char* source) const {
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

bool Renderer::createPointPipeline() {
    const unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, kPointVertexShaderSource);
    if (vertexShader == 0) {
        return false;
    }

    const unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, kPointFragmentShaderSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    m_pointProgram = glCreateProgram();
    glAttachShader(m_pointProgram, vertexShader);
    glAttachShader(m_pointProgram, fragmentShader);
    glLinkProgram(m_pointProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int linkSuccess = 0;
    glGetProgramiv(m_pointProgram, GL_LINK_STATUS, &linkSuccess);

    if (linkSuccess == GL_FALSE) {
        int logLength = 0;
        glGetProgramiv(m_pointProgram, GL_INFO_LOG_LENGTH, &logLength);

        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetProgramInfoLog(m_pointProgram, logLength, nullptr, log.data());

        std::cerr << "Program linking failed:\n" << log << '\n';

        glDeleteProgram(m_pointProgram);
        m_pointProgram = 0;
        return false;
    }

    constexpr std::array<float, 6> pointVertices {
        -0.5f, -0.2f,
         0.0f,  0.4f,
         0.5f, -0.2f
    };

    glGenVertexArrays(1, &m_pointVao);
    glGenBuffers(1, &m_pointVbo);

    glBindVertexArray(m_pointVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_pointVbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(pointVertices.size() * sizeof(float)),
        pointVertices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(2 * sizeof(float)),
        nullptr
    );
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}