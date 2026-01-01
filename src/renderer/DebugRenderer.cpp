#include "DebugRenderer.h"
#include <iostream>

namespace Genesis {

DebugRenderer::DebugRenderer() = default;

DebugRenderer::~DebugRenderer() {
    Shutdown();
}

bool DebugRenderer::Initialize() {
    if (m_initialized) return true;

    CreateBuffers();
    m_initialized = true;

    std::cout << "[DebugRenderer] Initialized" << std::endl;
    return true;
}

void DebugRenderer::Shutdown() {
    if (!m_initialized) return;

    if (m_lineVAO) {
        glDeleteVertexArrays(1, &m_lineVAO);
        m_lineVAO = 0;
    }
    if (m_lineVBO) {
        glDeleteBuffers(1, &m_lineVBO);
        m_lineVBO = 0;
    }
    if (m_triVAO) {
        glDeleteVertexArrays(1, &m_triVAO);
        m_triVAO = 0;
    }
    if (m_triVBO) {
        glDeleteBuffers(1, &m_triVBO);
        m_triVBO = 0;
    }

    m_initialized = false;
}

void DebugRenderer::CreateBuffers() {
    // Line VAO/VBO
    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Triangle VAO/VBO
    glGenVertexArrays(1, &m_triVAO);
    glGenBuffers(1, &m_triVBO);

    glBindVertexArray(m_triVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_triVBO);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void DebugRenderer::BeginFrame() {
    m_lineVertices.clear();
    m_triVertices.clear();
}

void DebugRenderer::DrawLine(float x1, float y1, float z1, float x2, float y2, float z2,
                              float r, float g, float b) {
    m_lineVertices.emplace_back(x1, y1, z1, r, g, b);
    m_lineVertices.emplace_back(x2, y2, z2, r, g, b);
}

void DebugRenderer::DrawGrid(float size, float spacing, float r, float g, float b) {
    float halfSize = size / 2.0f;
    int lines = static_cast<int>(size / spacing);

    for (int i = -lines / 2; i <= lines / 2; i++) {
        float pos = i * spacing;

        // Skip center lines (they'll be drawn as axes)
        if (i == 0) continue;

        // Lines parallel to X axis
        DrawLine(-halfSize, 0, pos, halfSize, 0, pos, r, g, b);

        // Lines parallel to Z axis
        DrawLine(pos, 0, -halfSize, pos, 0, halfSize, r, g, b);
    }
}

void DebugRenderer::DrawAxes(float length) {
    // X axis - Red
    DrawLine(0, 0, 0, length, 0, 0, 1.0f, 0.2f, 0.2f);
    // Small arrow head for X
    DrawLine(length, 0, 0, length - 0.2f, 0.1f, 0, 1.0f, 0.2f, 0.2f);
    DrawLine(length, 0, 0, length - 0.2f, -0.1f, 0, 1.0f, 0.2f, 0.2f);

    // Y axis - Green
    DrawLine(0, 0, 0, 0, length, 0, 0.2f, 1.0f, 0.2f);
    // Small arrow head for Y
    DrawLine(0, length, 0, 0.1f, length - 0.2f, 0, 0.2f, 1.0f, 0.2f);
    DrawLine(0, length, 0, -0.1f, length - 0.2f, 0, 0.2f, 1.0f, 0.2f);

    // Z axis - Blue
    DrawLine(0, 0, 0, 0, 0, length, 0.2f, 0.2f, 1.0f);
    // Small arrow head for Z
    DrawLine(0, 0, length, 0, 0.1f, length - 0.2f, 0.2f, 0.2f, 1.0f);
    DrawLine(0, 0, length, 0, -0.1f, length - 0.2f, 0.2f, 0.2f, 1.0f);

    // Negative axes (dimmer)
    DrawLine(0, 0, 0, -length * 0.5f, 0, 0, 0.5f, 0.1f, 0.1f);  // -X
    DrawLine(0, 0, 0, 0, -length * 0.5f, 0, 0.1f, 0.5f, 0.1f);  // -Y
    DrawLine(0, 0, 0, 0, 0, -length * 0.5f, 0.1f, 0.1f, 0.5f);  // -Z
}

void DebugRenderer::DrawCube(float x, float y, float z, float size, float r, float g, float b) {
    float hs = size / 2.0f;  // half size

    // Front face
    m_triVertices.emplace_back(x - hs, y - hs, z + hs, r, g, b);
    m_triVertices.emplace_back(x + hs, y - hs, z + hs, r, g, b);
    m_triVertices.emplace_back(x + hs, y + hs, z + hs, r, g, b);
    m_triVertices.emplace_back(x - hs, y - hs, z + hs, r, g, b);
    m_triVertices.emplace_back(x + hs, y + hs, z + hs, r, g, b);
    m_triVertices.emplace_back(x - hs, y + hs, z + hs, r, g, b);

    // Back face
    m_triVertices.emplace_back(x + hs, y - hs, z - hs, r * 0.8f, g * 0.8f, b * 0.8f);
    m_triVertices.emplace_back(x - hs, y - hs, z - hs, r * 0.8f, g * 0.8f, b * 0.8f);
    m_triVertices.emplace_back(x - hs, y + hs, z - hs, r * 0.8f, g * 0.8f, b * 0.8f);
    m_triVertices.emplace_back(x + hs, y - hs, z - hs, r * 0.8f, g * 0.8f, b * 0.8f);
    m_triVertices.emplace_back(x - hs, y + hs, z - hs, r * 0.8f, g * 0.8f, b * 0.8f);
    m_triVertices.emplace_back(x + hs, y + hs, z - hs, r * 0.8f, g * 0.8f, b * 0.8f);

    // Top face
    m_triVertices.emplace_back(x - hs, y + hs, z + hs, r * 1.0f, g * 1.0f, b * 1.0f);
    m_triVertices.emplace_back(x + hs, y + hs, z + hs, r * 1.0f, g * 1.0f, b * 1.0f);
    m_triVertices.emplace_back(x + hs, y + hs, z - hs, r * 1.0f, g * 1.0f, b * 1.0f);
    m_triVertices.emplace_back(x - hs, y + hs, z + hs, r * 1.0f, g * 1.0f, b * 1.0f);
    m_triVertices.emplace_back(x + hs, y + hs, z - hs, r * 1.0f, g * 1.0f, b * 1.0f);
    m_triVertices.emplace_back(x - hs, y + hs, z - hs, r * 1.0f, g * 1.0f, b * 1.0f);

    // Bottom face
    m_triVertices.emplace_back(x - hs, y - hs, z - hs, r * 0.6f, g * 0.6f, b * 0.6f);
    m_triVertices.emplace_back(x + hs, y - hs, z - hs, r * 0.6f, g * 0.6f, b * 0.6f);
    m_triVertices.emplace_back(x + hs, y - hs, z + hs, r * 0.6f, g * 0.6f, b * 0.6f);
    m_triVertices.emplace_back(x - hs, y - hs, z - hs, r * 0.6f, g * 0.6f, b * 0.6f);
    m_triVertices.emplace_back(x + hs, y - hs, z + hs, r * 0.6f, g * 0.6f, b * 0.6f);
    m_triVertices.emplace_back(x - hs, y - hs, z + hs, r * 0.6f, g * 0.6f, b * 0.6f);

    // Right face
    m_triVertices.emplace_back(x + hs, y - hs, z + hs, r * 0.9f, g * 0.9f, b * 0.9f);
    m_triVertices.emplace_back(x + hs, y - hs, z - hs, r * 0.9f, g * 0.9f, b * 0.9f);
    m_triVertices.emplace_back(x + hs, y + hs, z - hs, r * 0.9f, g * 0.9f, b * 0.9f);
    m_triVertices.emplace_back(x + hs, y - hs, z + hs, r * 0.9f, g * 0.9f, b * 0.9f);
    m_triVertices.emplace_back(x + hs, y + hs, z - hs, r * 0.9f, g * 0.9f, b * 0.9f);
    m_triVertices.emplace_back(x + hs, y + hs, z + hs, r * 0.9f, g * 0.9f, b * 0.9f);

    // Left face
    m_triVertices.emplace_back(x - hs, y - hs, z - hs, r * 0.7f, g * 0.7f, b * 0.7f);
    m_triVertices.emplace_back(x - hs, y - hs, z + hs, r * 0.7f, g * 0.7f, b * 0.7f);
    m_triVertices.emplace_back(x - hs, y + hs, z + hs, r * 0.7f, g * 0.7f, b * 0.7f);
    m_triVertices.emplace_back(x - hs, y - hs, z - hs, r * 0.7f, g * 0.7f, b * 0.7f);
    m_triVertices.emplace_back(x - hs, y + hs, z + hs, r * 0.7f, g * 0.7f, b * 0.7f);
    m_triVertices.emplace_back(x - hs, y + hs, z - hs, r * 0.7f, g * 0.7f, b * 0.7f);
}

void DebugRenderer::DrawWireCube(float x, float y, float z, float size, float r, float g, float b) {
    float hs = size / 2.0f;

    // Bottom face edges
    DrawLine(x - hs, y - hs, z - hs, x + hs, y - hs, z - hs, r, g, b);
    DrawLine(x + hs, y - hs, z - hs, x + hs, y - hs, z + hs, r, g, b);
    DrawLine(x + hs, y - hs, z + hs, x - hs, y - hs, z + hs, r, g, b);
    DrawLine(x - hs, y - hs, z + hs, x - hs, y - hs, z - hs, r, g, b);

    // Top face edges
    DrawLine(x - hs, y + hs, z - hs, x + hs, y + hs, z - hs, r, g, b);
    DrawLine(x + hs, y + hs, z - hs, x + hs, y + hs, z + hs, r, g, b);
    DrawLine(x + hs, y + hs, z + hs, x - hs, y + hs, z + hs, r, g, b);
    DrawLine(x - hs, y + hs, z + hs, x - hs, y + hs, z - hs, r, g, b);

    // Vertical edges
    DrawLine(x - hs, y - hs, z - hs, x - hs, y + hs, z - hs, r, g, b);
    DrawLine(x + hs, y - hs, z - hs, x + hs, y + hs, z - hs, r, g, b);
    DrawLine(x + hs, y - hs, z + hs, x + hs, y + hs, z + hs, r, g, b);
    DrawLine(x - hs, y - hs, z + hs, x - hs, y + hs, z + hs, r, g, b);
}

void DebugRenderer::DrawFloor(float size, float y, float r, float g, float b) {
    float hs = size / 2.0f;

    // Two triangles for the floor quad
    m_triVertices.emplace_back(-hs, y, -hs, r, g, b);
    m_triVertices.emplace_back( hs, y, -hs, r, g, b);
    m_triVertices.emplace_back( hs, y,  hs, r, g, b);

    m_triVertices.emplace_back(-hs, y, -hs, r, g, b);
    m_triVertices.emplace_back( hs, y,  hs, r, g, b);
    m_triVertices.emplace_back(-hs, y,  hs, r, g, b);
}

void DebugRenderer::UpdateLineBuffer() {
    if (m_lineVertices.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 m_lineVertices.size() * sizeof(Vertex),
                 m_lineVertices.data(),
                 GL_DYNAMIC_DRAW);
}

void DebugRenderer::UpdateTriangleBuffer() {
    if (m_triVertices.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_triVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 m_triVertices.size() * sizeof(Vertex),
                 m_triVertices.data(),
                 GL_DYNAMIC_DRAW);
}

void DebugRenderer::RenderLines() {
    if (m_lineVertices.empty()) return;

    UpdateLineBuffer();

    glBindVertexArray(m_lineVAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_lineVertices.size()));
    glBindVertexArray(0);
}

void DebugRenderer::RenderTriangles() {
    if (m_triVertices.empty()) return;

    UpdateTriangleBuffer();

    glBindVertexArray(m_triVAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_triVertices.size()));
    glBindVertexArray(0);
}

void DebugRenderer::EndFrame() {
    // Nothing special needed here for now
}

} // namespace Genesis

