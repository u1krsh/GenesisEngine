#pragma once

#include "VertexLayout.h"
#include "math/Math.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace Genesis {

// Forward declarations
class Shader;

// ============================================================================
// Draw Mode - How to interpret the vertex data
// ============================================================================
enum class DrawMode {
    Triangles,
    TriangleStrip,
    TriangleFan,
    Lines,
    LineStrip,
    LineLoop,
    Points
};

// ============================================================================
// Index Type - Size of index data
// ============================================================================
enum class IndexType {
    None,       // No indices (draw arrays)
    UInt16,     // 16-bit indices (up to 65535 vertices)
    UInt32      // 32-bit indices (up to 4 billion vertices)
};

// ============================================================================
// Mesh - Engine-grade mesh abstraction
//
// Owns vertex data, index data, and GPU resources (VAO/VBO/EBO).
// Provides a simple interface for the game to use meshes without
// touching OpenGL directly.
// ============================================================================
class Mesh {
public:
    Mesh();
    Mesh(const std::string& name);
    ~Mesh();

    // Non-copyable, but movable
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // ========================================================================
    // Setup - Call these before Upload()
    // ========================================================================

    // Set the vertex layout (describes format of vertex data)
    void SetLayout(const VertexLayout& layout);

    // Set vertex data (raw bytes, must match layout)
    void SetVertexData(const void* data, size_t sizeBytes, size_t vertexCount);

    // Set vertex data from vector (convenience template)
    template<typename T>
    void SetVertexData(const std::vector<T>& vertices) {
        SetVertexData(vertices.data(), vertices.size() * sizeof(T), vertices.size());
    }

    // Set index data (16-bit)
    void SetIndexData(const std::vector<uint16_t>& indices);

    // Set index data (32-bit)
    void SetIndexData(const std::vector<uint32_t>& indices);

    // Set draw mode
    void SetDrawMode(DrawMode mode) { m_drawMode = mode; }

    // ========================================================================
    // GPU Upload
    // ========================================================================

    // Upload data to GPU (creates VAO/VBO/EBO)
    bool Upload();

    // Re-upload vertex data (for dynamic meshes)
    bool UpdateVertexData(const void* data, size_t sizeBytes);

    template<typename T>
    bool UpdateVertexData(const std::vector<T>& vertices) {
        return UpdateVertexData(vertices.data(), vertices.size() * sizeof(T));
    }

    // Re-upload index data
    bool UpdateIndexData(const std::vector<uint16_t>& indices);
    bool UpdateIndexData(const std::vector<uint32_t>& indices);

    // Release GPU resources
    void Release();

    // ========================================================================
    // Drawing
    // ========================================================================

    // Bind the mesh (sets VAO)
    void Bind() const;

    // Unbind
    void Unbind() const;

    // Draw the mesh (just issues the draw call, you must bind shader first)
    void Draw() const;

    // Draw with instance count
    void DrawInstanced(uint32_t instanceCount) const;

    // Draw a subset
    void DrawRange(uint32_t startIndex, uint32_t count) const;

    // ========================================================================
    // Getters
    // ========================================================================

    const std::string& GetName() const { return m_name; }
    const VertexLayout& GetLayout() const { return m_layout; }
    uint32_t GetVertexCount() const { return m_vertexCount; }
    uint32_t GetIndexCount() const { return m_indexCount; }
    DrawMode GetDrawMode() const { return m_drawMode; }
    IndexType GetIndexType() const { return m_indexType; }
    bool IsUploaded() const { return m_vao != 0; }
    bool HasIndices() const { return m_indexType != IndexType::None && m_indexCount > 0; }

    // Get raw vertex data (CPU side)
    const std::vector<uint8_t>& GetVertexData() const { return m_vertexData; }

    // OpenGL handles (for advanced usage)
    uint32_t GetVAO() const { return m_vao; }
    uint32_t GetVBO() const { return m_vbo; }
    uint32_t GetEBO() const { return m_ebo; }

    // ========================================================================
    // Bounding Volume (for culling)
    // ========================================================================

    void SetBoundingBox(const Vec3& min, const Vec3& max);
    void CalculateBoundingBox();  // Auto-calculate from vertex data (assumes Position3D is first)

    const Vec3& GetBoundsMin() const { return m_boundsMin; }
    const Vec3& GetBoundsMax() const { return m_boundsMax; }
    Vec3 GetBoundsCenter() const { return (m_boundsMin + m_boundsMax) * 0.5f; }
    Vec3 GetBoundsExtents() const { return (m_boundsMax - m_boundsMin) * 0.5f; }

private:
    void SetupVertexAttributes();
    GLenum GetGLDrawMode() const;
    GLenum GetGLIndexType() const;

private:
    std::string m_name;
    VertexLayout m_layout;
    DrawMode m_drawMode = DrawMode::Triangles;

    // CPU-side data
    std::vector<uint8_t> m_vertexData;
    std::vector<uint8_t> m_indexData;  // Can be uint16 or uint32
    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;
    IndexType m_indexType = IndexType::None;

    // GPU handles
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;

    // Bounding volume
    Vec3 m_boundsMin = Vec3(0.0f);
    Vec3 m_boundsMax = Vec3(0.0f);
};

// ============================================================================
// Shared Mesh Pointer
// ============================================================================
using MeshPtr = std::shared_ptr<Mesh>;

} // namespace Genesis

