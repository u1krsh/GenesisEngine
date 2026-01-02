#pragma once

#include "math/Math.h"
#include <string>
#include <variant>
#include <unordered_map>
#include <memory>

namespace Genesis {

// Forward declarations
class Texture2D;

// ============================================================================
// Material Property Types - All possible property value types
// ============================================================================
enum class MaterialPropertyType {
    None,
    Int,
    Float,
    Vec2,
    Vec3,
    Vec4,
    Mat3,
    Mat4,
    Texture2D
};

// ============================================================================
// Texture Slot - Represents a texture binding with settings
// ============================================================================
struct TextureSlot {
    std::shared_ptr<Texture2D> texture = nullptr;
    int unit = 0;           // Texture unit to bind to
    std::string uniformName; // Name in shader (e.g., "u_DiffuseMap")

    // Tiling/offset
    Vec2 tiling = Vec2(1.0f, 1.0f);
    Vec2 offset = Vec2(0.0f, 0.0f);
};

// ============================================================================
// Material Property Value - Type-safe variant for all property types
// ============================================================================
using MaterialPropertyValue = std::variant<
    std::monostate,     // None/empty
    int,                // Int
    float,              // Float
    Vec2,               // Vec2
    Vec3,               // Vec3
    Vec4,               // Vec4
    Mat3,               // Mat3
    Mat4,               // Mat4
    TextureSlot         // Texture with settings
>;

// ============================================================================
// Material Property - Named property with value
// ============================================================================
struct MaterialProperty {
    std::string name;
    MaterialPropertyType type = MaterialPropertyType::None;
    MaterialPropertyValue value;

    // Constructors for convenience
    MaterialProperty() = default;
    MaterialProperty(const std::string& n, int v)
        : name(n), type(MaterialPropertyType::Int), value(v) {}
    MaterialProperty(const std::string& n, float v)
        : name(n), type(MaterialPropertyType::Float), value(v) {}
    MaterialProperty(const std::string& n, const Vec2& v)
        : name(n), type(MaterialPropertyType::Vec2), value(v) {}
    MaterialProperty(const std::string& n, const Vec3& v)
        : name(n), type(MaterialPropertyType::Vec3), value(v) {}
    MaterialProperty(const std::string& n, const Vec4& v)
        : name(n), type(MaterialPropertyType::Vec4), value(v) {}
    MaterialProperty(const std::string& n, const Mat3& v)
        : name(n), type(MaterialPropertyType::Mat3), value(v) {}
    MaterialProperty(const std::string& n, const Mat4& v)
        : name(n), type(MaterialPropertyType::Mat4), value(v) {}
    MaterialProperty(const std::string& n, const TextureSlot& v)
        : name(n), type(MaterialPropertyType::Texture2D), value(v) {}
};

// ============================================================================
// Property Helpers
// ============================================================================
inline MaterialPropertyType GetPropertyType(const MaterialPropertyValue& value) {
    return std::visit([](auto&& arg) -> MaterialPropertyType {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) return MaterialPropertyType::None;
        else if constexpr (std::is_same_v<T, int>) return MaterialPropertyType::Int;
        else if constexpr (std::is_same_v<T, float>) return MaterialPropertyType::Float;
        else if constexpr (std::is_same_v<T, Vec2>) return MaterialPropertyType::Vec2;
        else if constexpr (std::is_same_v<T, Vec3>) return MaterialPropertyType::Vec3;
        else if constexpr (std::is_same_v<T, Vec4>) return MaterialPropertyType::Vec4;
        else if constexpr (std::is_same_v<T, Mat3>) return MaterialPropertyType::Mat3;
        else if constexpr (std::is_same_v<T, Mat4>) return MaterialPropertyType::Mat4;
        else if constexpr (std::is_same_v<T, TextureSlot>) return MaterialPropertyType::Texture2D;
        else return MaterialPropertyType::None;
    }, value);
}

} // namespace Genesis

