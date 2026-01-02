#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <glad/glad.h>

namespace Genesis {

// ============================================================================
// Vertex Attribute Types
// ============================================================================
enum class VertexAttribType {
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UByte4Norm  // 4 unsigned bytes normalized to 0-1 (for colors)
};

// ============================================================================
// Vertex Attribute - Describes a single attribute in a vertex
// ============================================================================
struct VertexAttribute {
    std::string name;           // For debugging/identification
    VertexAttribType type;      // Data type
    uint32_t offset;            // Byte offset within vertex (calculated automatically)
    bool normalized;            // Whether to normalize integer types

    VertexAttribute(const std::string& name, VertexAttribType type, bool normalized = false)
        : name(name), type(type), offset(0), normalized(normalized) {}

    // Get the number of components (e.g., Float3 = 3)
    uint32_t GetComponentCount() const {
        switch (type) {
            case VertexAttribType::Float:      return 1;
            case VertexAttribType::Float2:     return 2;
            case VertexAttribType::Float3:     return 3;
            case VertexAttribType::Float4:     return 4;
            case VertexAttribType::Int:        return 1;
            case VertexAttribType::Int2:       return 2;
            case VertexAttribType::Int3:       return 3;
            case VertexAttribType::Int4:       return 4;
            case VertexAttribType::UByte4Norm: return 4;
        }
        return 0;
    }

    // Get the size in bytes
    uint32_t GetSize() const {
        switch (type) {
            case VertexAttribType::Float:      return 4;
            case VertexAttribType::Float2:     return 8;
            case VertexAttribType::Float3:     return 12;
            case VertexAttribType::Float4:     return 16;
            case VertexAttribType::Int:        return 4;
            case VertexAttribType::Int2:       return 8;
            case VertexAttribType::Int3:       return 12;
            case VertexAttribType::Int4:       return 16;
            case VertexAttribType::UByte4Norm: return 4;
        }
        return 0;
    }

    // Get OpenGL type
    GLenum GetGLType() const {
        switch (type) {
            case VertexAttribType::Float:
            case VertexAttribType::Float2:
            case VertexAttribType::Float3:
            case VertexAttribType::Float4:
                return GL_FLOAT;
            case VertexAttribType::Int:
            case VertexAttribType::Int2:
            case VertexAttribType::Int3:
            case VertexAttribType::Int4:
                return GL_INT;
            case VertexAttribType::UByte4Norm:
                return GL_UNSIGNED_BYTE;
        }
        return GL_FLOAT;
    }
};

// ============================================================================
// Vertex Layout - Describes the complete format of a vertex
// ============================================================================
class VertexLayout {
public:
    VertexLayout() = default;

    // Add an attribute to the layout
    VertexLayout& Add(const std::string& name, VertexAttribType type, bool normalized = false) {
        VertexAttribute attrib(name, type, normalized);
        attrib.offset = m_stride;
        m_stride += attrib.GetSize();
        m_attributes.push_back(attrib);
        return *this;
    }

    // Convenience methods for common attributes
    VertexLayout& Position3D() { return Add("position", VertexAttribType::Float3); }
    VertexLayout& Position2D() { return Add("position", VertexAttribType::Float2); }
    VertexLayout& Normal() { return Add("normal", VertexAttribType::Float3); }
    VertexLayout& TexCoord() { return Add("texCoord", VertexAttribType::Float2); }
    VertexLayout& Color3() { return Add("color", VertexAttribType::Float3); }
    VertexLayout& Color4() { return Add("color", VertexAttribType::Float4); }
    VertexLayout& Tangent() { return Add("tangent", VertexAttribType::Float3); }
    VertexLayout& Bitangent() { return Add("bitangent", VertexAttribType::Float3); }

    // Getters
    const std::vector<VertexAttribute>& GetAttributes() const { return m_attributes; }
    uint32_t GetStride() const { return m_stride; }
    size_t GetAttributeCount() const { return m_attributes.size(); }

    // ========================================================================
    // Common Predefined Layouts
    // ========================================================================

    // Position only (3 floats)
    static VertexLayout P3() {
        return VertexLayout().Position3D();
    }

    // Position + Color (3 + 3 floats)
    static VertexLayout P3C3() {
        return VertexLayout().Position3D().Color3();
    }

    // Position + TexCoord (3 + 2 floats)
    static VertexLayout P3T2() {
        return VertexLayout().Position3D().TexCoord();
    }

    // Position + Normal (3 + 3 floats)
    static VertexLayout P3N3() {
        return VertexLayout().Position3D().Normal();
    }

    // Position + Normal + TexCoord (3 + 3 + 2 floats) - Most common
    static VertexLayout P3N3T2() {
        return VertexLayout().Position3D().Normal().TexCoord();
    }

    // Position + Normal + TexCoord + Tangent (for normal mapping)
    static VertexLayout P3N3T2Tan3() {
        return VertexLayout().Position3D().Normal().TexCoord().Tangent();
    }

    // Full vertex with tangent and bitangent
    static VertexLayout P3N3T2Tan3Bit3() {
        return VertexLayout().Position3D().Normal().TexCoord().Tangent().Bitangent();
    }

private:
    std::vector<VertexAttribute> m_attributes;
    uint32_t m_stride = 0;
};

} // namespace Genesis

