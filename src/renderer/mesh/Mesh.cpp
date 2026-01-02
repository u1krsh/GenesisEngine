#include "Mesh.h"
#include <glad/glad.h>
#include <cstring>
#include <limits>
#include <iostream>

namespace Genesis {

// ============================================================================
// Constructor / Destructor
// ============================================================================

Mesh::Mesh() : m_name("Unnamed") {}

Mesh::Mesh(const std::string& name) : m_name(name) {}

Mesh::~Mesh() {
    Release();
}

// ============================================================================
// Move Semantics
// ============================================================================

Mesh::Mesh(Mesh&& other) noexcept
    : m_name(std::move(other.m_name))
    , m_layout(std::move(other.m_layout))
    , m_drawMode(other.m_drawMode)
    , m_vertexData(std::move(other.m_vertexData))
    , m_indexData(std::move(other.m_indexData))
    , m_vertexCount(other.m_vertexCount)
    , m_indexCount(other.m_indexCount)
    , m_indexType(other.m_indexType)
    , m_vao(other.m_vao)
    , m_vbo(other.m_vbo)
    , m_ebo(other.m_ebo)
    , m_boundsMin(other.m_boundsMin)
    , m_boundsMax(other.m_boundsMax)
{
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
    other.m_vertexCount = 0;
    other.m_indexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        Release();

        m_name = std::move(other.m_name);
        m_layout = std::move(other.m_layout);
        m_drawMode = other.m_drawMode;
        m_vertexData = std::move(other.m_vertexData);
        m_indexData = std::move(other.m_indexData);
        m_vertexCount = other.m_vertexCount;
        m_indexCount = other.m_indexCount;
        m_indexType = other.m_indexType;
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_boundsMin = other.m_boundsMin;
        m_boundsMax = other.m_boundsMax;

        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_vertexCount = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

// ============================================================================
// Setup
// ============================================================================

void Mesh::SetLayout(const VertexLayout& layout) {
    m_layout = layout;
}

void Mesh::SetVertexData(const void* data, size_t sizeBytes, size_t vertexCount) {
    m_vertexData.resize(sizeBytes);
    std::memcpy(m_vertexData.data(), data, sizeBytes);
    m_vertexCount = static_cast<uint32_t>(vertexCount);
}

void Mesh::SetIndexData(const std::vector<uint16_t>& indices) {
    m_indexData.resize(indices.size() * sizeof(uint16_t));
    std::memcpy(m_indexData.data(), indices.data(), m_indexData.size());
    m_indexCount = static_cast<uint32_t>(indices.size());
    m_indexType = IndexType::UInt16;
}

void Mesh::SetIndexData(const std::vector<uint32_t>& indices) {
    m_indexData.resize(indices.size() * sizeof(uint32_t));
    std::memcpy(m_indexData.data(), indices.data(), m_indexData.size());
    m_indexCount = static_cast<uint32_t>(indices.size());
    m_indexType = IndexType::UInt32;
}

// ============================================================================
// GPU Upload
// ============================================================================

bool Mesh::Upload() {
    if (m_vertexData.empty()) {
        std::cerr << "[Mesh] Cannot upload mesh '" << m_name << "': no vertex data" << std::endl;
        return false;
    }

    if (m_layout.GetAttributeCount() == 0) {
        std::cerr << "[Mesh] Cannot upload mesh '" << m_name << "': no vertex layout defined" << std::endl;
        return false;
    }

    // Release any existing resources
    Release();

    // Create VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Create VBO
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexData.size(), m_vertexData.data(), GL_STATIC_DRAW);

    // Setup vertex attributes
    SetupVertexAttributes();

    // Create EBO if we have indices
    if (!m_indexData.empty()) {
        glGenBuffers(1, &m_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexData.size(), m_indexData.data(), GL_STATIC_DRAW);
    }

    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Note: Don't unbind EBO while VAO is bound, it's part of VAO state

    return true;
}

bool Mesh::UpdateVertexData(const void* data, size_t sizeBytes) {
    if (m_vbo == 0) {
        std::cerr << "[Mesh] Cannot update vertex data: mesh not uploaded" << std::endl;
        return false;
    }

    // Update CPU-side data
    if (sizeBytes != m_vertexData.size()) {
        m_vertexData.resize(sizeBytes);
    }
    std::memcpy(m_vertexData.data(), data, sizeBytes);

    // Update GPU buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeBytes, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

bool Mesh::UpdateIndexData(const std::vector<uint16_t>& indices) {
    if (m_ebo == 0) {
        std::cerr << "[Mesh] Cannot update index data: no EBO" << std::endl;
        return false;
    }

    m_indexData.resize(indices.size() * sizeof(uint16_t));
    std::memcpy(m_indexData.data(), indices.data(), m_indexData.size());
    m_indexCount = static_cast<uint32_t>(indices.size());
    m_indexType = IndexType::UInt16;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indexData.size(), m_indexData.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return true;
}

bool Mesh::UpdateIndexData(const std::vector<uint32_t>& indices) {
    if (m_ebo == 0) {
        std::cerr << "[Mesh] Cannot update index data: no EBO" << std::endl;
        return false;
    }

    m_indexData.resize(indices.size() * sizeof(uint32_t));
    std::memcpy(m_indexData.data(), indices.data(), m_indexData.size());
    m_indexCount = static_cast<uint32_t>(indices.size());
    m_indexType = IndexType::UInt32;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indexData.size(), m_indexData.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return true;
}

void Mesh::Release() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
}

// ============================================================================
// Drawing
// ============================================================================

void Mesh::Bind() const {
    if (m_vao != 0) {
        glBindVertexArray(m_vao);
    }
}

void Mesh::Unbind() const {
    glBindVertexArray(0);
}

void Mesh::Draw() const {
    if (m_vao == 0) {
        std::cerr << "[Mesh] Attempting to draw mesh '" << m_name << "' that hasn't been uploaded" << std::endl;
        return;
    }

    glBindVertexArray(m_vao);

    if (HasIndices()) {
        glDrawElements(GetGLDrawMode(), m_indexCount, GetGLIndexType(), nullptr);
    } else {
        glDrawArrays(GetGLDrawMode(), 0, m_vertexCount);
    }

    glBindVertexArray(0);
}

void Mesh::DrawInstanced(uint32_t instanceCount) const {
    if (m_vao == 0) return;

    glBindVertexArray(m_vao);

    if (HasIndices()) {
        glDrawElementsInstanced(GetGLDrawMode(), m_indexCount, GetGLIndexType(), nullptr, instanceCount);
    } else {
        glDrawArraysInstanced(GetGLDrawMode(), 0, m_vertexCount, instanceCount);
    }

    glBindVertexArray(0);
}

void Mesh::DrawRange(uint32_t startIndex, uint32_t count) const {
    if (m_vao == 0) return;

    glBindVertexArray(m_vao);

    if (HasIndices()) {
        size_t offset = startIndex * (m_indexType == IndexType::UInt16 ? 2 : 4);
        glDrawElements(GetGLDrawMode(), count, GetGLIndexType(), reinterpret_cast<void*>(offset));
    } else {
        glDrawArrays(GetGLDrawMode(), startIndex, count);
    }

    glBindVertexArray(0);
}

// ============================================================================
// Bounding Volume
// ============================================================================

void Mesh::SetBoundingBox(const Vec3& min, const Vec3& max) {
    m_boundsMin = min;
    m_boundsMax = max;
}

void Mesh::CalculateBoundingBox() {
    if (m_vertexData.empty() || m_layout.GetStride() < 12) {
        return;  // Need at least 3 floats for position
    }

    m_boundsMin = Vec3(std::numeric_limits<float>::max());
    m_boundsMax = Vec3(std::numeric_limits<float>::lowest());

    const uint8_t* data = m_vertexData.data();
    uint32_t stride = m_layout.GetStride();

    for (uint32_t i = 0; i < m_vertexCount; ++i) {
        const float* pos = reinterpret_cast<const float*>(data + i * stride);
        Vec3 position(pos[0], pos[1], pos[2]);

        m_boundsMin = glm::min(m_boundsMin, position);
        m_boundsMax = glm::max(m_boundsMax, position);
    }
}

// ============================================================================
// Private Helpers
// ============================================================================

void Mesh::SetupVertexAttributes() {
    const auto& attributes = m_layout.GetAttributes();
    uint32_t stride = m_layout.GetStride();

    for (size_t i = 0; i < attributes.size(); ++i) {
        const auto& attrib = attributes[i];

        glEnableVertexAttribArray(static_cast<GLuint>(i));

        if (attrib.type == VertexAttribType::Int ||
            attrib.type == VertexAttribType::Int2 ||
            attrib.type == VertexAttribType::Int3 ||
            attrib.type == VertexAttribType::Int4) {
            // Integer attributes
            glVertexAttribIPointer(
                static_cast<GLuint>(i),
                attrib.GetComponentCount(),
                attrib.GetGLType(),
                stride,
                reinterpret_cast<void*>(static_cast<uintptr_t>(attrib.offset))
            );
        } else {
            // Float attributes (including normalized)
            glVertexAttribPointer(
                static_cast<GLuint>(i),
                attrib.GetComponentCount(),
                attrib.GetGLType(),
                attrib.normalized || attrib.type == VertexAttribType::UByte4Norm ? GL_TRUE : GL_FALSE,
                stride,
                reinterpret_cast<void*>(static_cast<uintptr_t>(attrib.offset))
            );
        }
    }
}

GLenum Mesh::GetGLDrawMode() const {
    switch (m_drawMode) {
        case DrawMode::Triangles:     return GL_TRIANGLES;
        case DrawMode::TriangleStrip: return GL_TRIANGLE_STRIP;
        case DrawMode::TriangleFan:   return GL_TRIANGLE_FAN;
        case DrawMode::Lines:         return GL_LINES;
        case DrawMode::LineStrip:     return GL_LINE_STRIP;
        case DrawMode::LineLoop:      return GL_LINE_LOOP;
        case DrawMode::Points:        return GL_POINTS;
    }
    return GL_TRIANGLES;
}

GLenum Mesh::GetGLIndexType() const {
    switch (m_indexType) {
        case IndexType::UInt16: return GL_UNSIGNED_SHORT;
        case IndexType::UInt32: return GL_UNSIGNED_INT;
        default: return GL_UNSIGNED_INT;
    }
}

} // namespace Genesis

