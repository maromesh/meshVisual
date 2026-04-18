#include "meshvisual/core/Renderer.hpp"

#include <OpenGL/gl3.h>

#include <iostream>
#include <span>
#include <string>

namespace {
constexpr const char* kPointVertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec2 aPosition;

uniform float uPointSize;

void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    gl_PointSize = uPointSize;
}
)";

constexpr const char* kPointFragmentShaderSource = R"(
#version 330 core

uniform vec4 uPointColor;

out vec4 fragColor;

void main() {
    vec2 centeredPointCoord = (gl_PointCoord * 2.0) - vec2(1.0);
    if (dot(centeredPointCoord, centeredPointCoord) > 1.0) {
        discard;
    }

    fragColor = uPointColor;
}
)";

constexpr const char* kLineVertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aLineVertex;

out float vAlpha;

void main() {
    gl_Position = vec4(aLineVertex.xy, 0.0, 1.0);
    vAlpha = aLineVertex.z;
}
)";

constexpr const char* kLineFragmentShaderSource = R"(
#version 330 core

in float vAlpha;

uniform vec4 uEdgeColor;

out vec4 fragColor;

void main() {
    fragColor = vec4(uEdgeColor.rgb, uEdgeColor.a * vAlpha);
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!createLinePipeline() || !createPointPipeline()) {
        destroy();
        return false;
    }

    m_initialized = true;
    return true;
}

void Renderer::configure(const AppSettings& settings) {
    m_pointSize = settings.pointSize;
    m_lineWidth = settings.lineWidth;
    m_pointColor = settings.pointColor;
    m_edgeColor = settings.edgeColor;
}

void Renderer::beginFrame(int width, int height) {
    glViewport(0, 0, width, height);
}

void Renderer::clear(float red, float green, float blue, float alpha) {
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::drawLines() {
    if (!m_initialized || m_lineProgram == 0 || m_lineVao == 0 || m_lineVertexCount <= 0) {
        return;
    }

    glUseProgram(m_lineProgram);
    glLineWidth(m_lineWidth);
    const int edgeColorLocation = glGetUniformLocation(m_lineProgram, "uEdgeColor");
    if (edgeColorLocation >= 0) {
        glUniform4f(edgeColorLocation, m_edgeColor.red, m_edgeColor.green, m_edgeColor.blue, m_edgeColor.alpha);
    }
    glBindVertexArray(m_lineVao);
    glDrawArrays(GL_LINES, 0, m_lineVertexCount);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::drawPoints() {
    if (!m_initialized || m_pointProgram == 0 || m_pointVao == 0 || m_pointCount <= 0) {
        return;
    }

    glUseProgram(m_pointProgram);
    const int pointSizeLocation = glGetUniformLocation(m_pointProgram, "uPointSize");
    if (pointSizeLocation >= 0) {
        glUniform1f(pointSizeLocation, m_pointSize);
    }
    const int pointColorLocation = glGetUniformLocation(m_pointProgram, "uPointColor");
    if (pointColorLocation >= 0) {
        glUniform4f(pointColorLocation, m_pointColor.red, m_pointColor.green, m_pointColor.blue, m_pointColor.alpha);
    }
    glBindVertexArray(m_pointVao);
    glDrawArrays(GL_POINTS, 0, m_pointCount);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::uploadLines(std::span<const float> lineVertices) {
    if (!m_initialized || m_lineVbo == 0) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_lineVbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(lineVertices.size_bytes()),
        lineVertices.data(),
        GL_STATIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_lineVertexCount = static_cast<int>(lineVertices.size() / 3U);
}

void Renderer::uploadPoints(std::span<const float> pointVertices) {
    if (!m_initialized || m_pointVbo == 0) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_pointVbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(pointVertices.size_bytes()),
        pointVertices.data(),
        GL_STATIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_pointCount = static_cast<int>(pointVertices.size() / 2U);
}

void Renderer::destroy() {
    if (m_lineVbo != 0) {
        glDeleteBuffers(1, &m_lineVbo);
        m_lineVbo = 0;
    }

    if (m_lineVao != 0) {
        glDeleteVertexArrays(1, &m_lineVao);
        m_lineVao = 0;
    }

    if (m_lineProgram != 0) {
        glDeleteProgram(m_lineProgram);
        m_lineProgram = 0;
    }

    if (m_pointVbo != 0) {
        glDeleteBuffers(1, &m_pointVbo);
        m_pointVbo = 0;
    }

    if (m_pointVao != 0) {
        glDeleteVertexArrays(1, &m_pointVao);
        m_pointVao = 0;
    }
    
    if (m_pointProgram != 0) {
        glDeleteProgram(m_pointProgram);
        m_pointProgram = 0;
    }

    m_lineVertexCount = 0;
    m_pointCount = 0;
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

    glGenVertexArrays(1, &m_pointVao);
    glGenBuffers(1, &m_pointVbo);

    glBindVertexArray(m_pointVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_pointVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

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

bool Renderer::createLinePipeline() {
    const unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, kLineVertexShaderSource);
    if (vertexShader == 0) {
        return false;
    }

    const unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, kLineFragmentShaderSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    m_lineProgram = glCreateProgram();
    glAttachShader(m_lineProgram, vertexShader);
    glAttachShader(m_lineProgram, fragmentShader);
    glLinkProgram(m_lineProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int linkSuccess = 0;
    glGetProgramiv(m_lineProgram, GL_LINK_STATUS, &linkSuccess);

    if (linkSuccess == GL_FALSE) {
        int logLength = 0;
        glGetProgramiv(m_lineProgram, GL_INFO_LOG_LENGTH, &logLength);

        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetProgramInfoLog(m_lineProgram, logLength, nullptr, log.data());

        std::cerr << "Program linking failed:\n" << log << '\n';

        glDeleteProgram(m_lineProgram);
        m_lineProgram = 0;
        return false;
    }

    glGenVertexArrays(1, &m_lineVao);
    glGenBuffers(1, &m_lineVbo);

    glBindVertexArray(m_lineVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(3 * sizeof(float)),
        nullptr
    );
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}
