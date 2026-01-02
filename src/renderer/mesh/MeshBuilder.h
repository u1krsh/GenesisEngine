#pragma once

#include "Mesh.h"
#include "VertexLayout.h"
#include "math/Math.h"
#include <vector>
#include <memory>

namespace Genesis {

// ============================================================================
// Standard Vertex Types - Common vertex formats for easy use
// ============================================================================

// Position only
struct VertexP {
    Vec3 position;

    VertexP() = default;
    VertexP(const Vec3& pos) : position(pos) {}
    VertexP(float x, float y, float z) : position(x, y, z) {}

    static VertexLayout GetLayout() { return VertexLayout::P3(); }
};

// Position + Color
struct VertexPC {
    Vec3 position;
    Vec3 color;

    VertexPC() = default;
    VertexPC(const Vec3& pos, const Vec3& col) : position(pos), color(col) {}
    VertexPC(float x, float y, float z, float r, float g, float b)
        : position(x, y, z), color(r, g, b) {}

    static VertexLayout GetLayout() { return VertexLayout::P3C3(); }
};

// Position + TexCoord
struct VertexPT {
    Vec3 position;
    Vec2 texCoord;

    VertexPT() = default;
    VertexPT(const Vec3& pos, const Vec2& uv) : position(pos), texCoord(uv) {}
    VertexPT(float x, float y, float z, float u, float v)
        : position(x, y, z), texCoord(u, v) {}

    static VertexLayout GetLayout() { return VertexLayout::P3T2(); }
};

// Position + Normal
struct VertexPN {
    Vec3 position;
    Vec3 normal;

    VertexPN() = default;
    VertexPN(const Vec3& pos, const Vec3& norm) : position(pos), normal(norm) {}

    static VertexLayout GetLayout() { return VertexLayout::P3N3(); }
};

// Position + Normal + TexCoord (most common)
struct VertexPNT {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;

    VertexPNT() = default;
    VertexPNT(const Vec3& pos, const Vec3& norm, const Vec2& uv)
        : position(pos), normal(norm), texCoord(uv) {}
    VertexPNT(float x, float y, float z, float nx, float ny, float nz, float u, float v)
        : position(x, y, z), normal(nx, ny, nz), texCoord(u, v) {}

    static VertexLayout GetLayout() { return VertexLayout::P3N3T2(); }
};

// Full vertex with tangent for normal mapping
struct VertexPNTTan {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;
    Vec3 tangent;

    VertexPNTTan() = default;
    VertexPNTTan(const Vec3& pos, const Vec3& norm, const Vec2& uv, const Vec3& tan)
        : position(pos), normal(norm), texCoord(uv), tangent(tan) {}

    static VertexLayout GetLayout() { return VertexLayout::P3N3T2Tan3(); }
};

// ============================================================================
// MeshBuilder - Fluent interface for building meshes
// ============================================================================
template<typename VertexType>
class MeshBuilder {
public:
    MeshBuilder(const std::string& name = "BuiltMesh") : m_name(name) {}

    // Add a single vertex
    MeshBuilder& AddVertex(const VertexType& vertex) {
        m_vertices.push_back(vertex);
        return *this;
    }

    // Add multiple vertices
    MeshBuilder& AddVertices(const std::vector<VertexType>& vertices) {
        m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
        return *this;
    }

    // Add a triangle (3 indices)
    MeshBuilder& AddTriangle(uint32_t i0, uint32_t i1, uint32_t i2) {
        m_indices.push_back(i0);
        m_indices.push_back(i1);
        m_indices.push_back(i2);
        return *this;
    }

    // Add a quad (4 indices, converted to 2 triangles)
    MeshBuilder& AddQuad(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3) {
        // First triangle
        m_indices.push_back(i0);
        m_indices.push_back(i1);
        m_indices.push_back(i2);
        // Second triangle
        m_indices.push_back(i0);
        m_indices.push_back(i2);
        m_indices.push_back(i3);
        return *this;
    }

    // Add raw indices
    MeshBuilder& AddIndices(const std::vector<uint32_t>& indices) {
        m_indices.insert(m_indices.end(), indices.begin(), indices.end());
        return *this;
    }

    // Set draw mode
    MeshBuilder& SetDrawMode(DrawMode mode) {
        m_drawMode = mode;
        return *this;
    }

    // Get current vertex count (useful for indexing)
    uint32_t GetVertexCount() const { return static_cast<uint32_t>(m_vertices.size()); }

    // Build and upload the mesh
    MeshPtr Build() {
        auto mesh = std::make_shared<Mesh>(m_name);
        mesh->SetLayout(VertexType::GetLayout());
        mesh->SetVertexData(m_vertices);

        if (!m_indices.empty()) {
            mesh->SetIndexData(m_indices);
        }

        mesh->SetDrawMode(m_drawMode);
        mesh->CalculateBoundingBox();
        mesh->Upload();

        return mesh;
    }

    // Build without uploading (for manual control)
    MeshPtr BuildNoUpload() {
        auto mesh = std::make_shared<Mesh>(m_name);
        mesh->SetLayout(VertexType::GetLayout());
        mesh->SetVertexData(m_vertices);

        if (!m_indices.empty()) {
            mesh->SetIndexData(m_indices);
        }

        mesh->SetDrawMode(m_drawMode);
        mesh->CalculateBoundingBox();

        return mesh;
    }

    // Clear the builder for reuse
    void Clear() {
        m_vertices.clear();
        m_indices.clear();
    }

private:
    std::string m_name;
    std::vector<VertexType> m_vertices;
    std::vector<uint32_t> m_indices;
    DrawMode m_drawMode = DrawMode::Triangles;
};

// ============================================================================
// Type aliases for common builders
// ============================================================================
using MeshBuilderP = MeshBuilder<VertexP>;
using MeshBuilderPC = MeshBuilder<VertexPC>;
using MeshBuilderPT = MeshBuilder<VertexPT>;
using MeshBuilderPN = MeshBuilder<VertexPN>;
using MeshBuilderPNT = MeshBuilder<VertexPNT>;

} // namespace Genesis

