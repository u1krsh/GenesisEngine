#pragma once

#include "MaterialProperty.h"
#include "renderer/shader/Shader.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Genesis {

// Forward declarations
class Texture2D;

// ============================================================================
// Render Queue - Controls rendering order
// ============================================================================
enum class RenderQueue {
    Background = 1000,      // Skybox, far background
    Geometry = 2000,        // Opaque geometry (default)
    AlphaTest = 2450,       // Alpha-tested geometry
    Transparent = 3000,     // Transparent geometry
    Overlay = 4000          // UI, debug overlays
};

// ============================================================================
// Blend Mode - How the material blends with background
// ============================================================================
enum class BlendMode {
    Opaque,             // No blending
    Cutout,             // Alpha test (discard below threshold)
    Transparent,        // Alpha blending
    Additive,           // Additive blending
    Multiply            // Multiplicative blending
};

// ============================================================================
// Cull Mode - Face culling
// ============================================================================
enum class CullMode {
    Off,                // No culling (double-sided)
    Back,               // Cull back faces (default)
    Front               // Cull front faces
};

// ============================================================================
// Material - Defines visual properties for rendering
//
// A Material references a Shader and holds property values (colors, textures,
// floats, etc.) that get uploaded to the shader when the material is bound.
//
// Design follows Source engine philosophy:
// - Shader defines the "type" of surface (LitOpaque, Unlit, etc.)
// - Material defines the "instance" (red metal, blue plastic, etc.)
// - One shader, many materials, zero duplicated logic
// ============================================================================
class Material {
public:
    Material();
    explicit Material(const std::string& name);
    Material(const std::string& name, std::shared_ptr<Shader> shader);
    ~Material() = default;

    // Non-copyable but movable
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&&) = default;
    Material& operator=(Material&&) = default;

    // ========================================================================
    // Shader Management
    // ========================================================================

    void SetShader(std::shared_ptr<Shader> shader);
    std::shared_ptr<Shader> GetShader() const { return m_shader; }
    bool HasShader() const { return m_shader != nullptr && m_shader->IsValid(); }

    // ========================================================================
    // Property Setters - Set uniform values by name
    // ========================================================================

    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVec2(const std::string& name, const Vec2& value);
    void SetVec3(const std::string& name, const Vec3& value);
    void SetVec4(const std::string& name, const Vec4& value);
    void SetMat3(const std::string& name, const Mat3& value);
    void SetMat4(const std::string& name, const Mat4& value);
    void SetTexture(const std::string& name, std::shared_ptr<Texture2D> texture, int unit = 0);
    void SetTextureSlot(const std::string& name, const TextureSlot& slot);

    // Convenience setters for common properties
    void SetColor(const Vec3& color) { SetVec3("u_Color", color); }
    void SetColor(const Vec4& color) { SetVec4("u_Color", color); }
    void SetColor(float r, float g, float b) { SetVec3("u_Color", Vec3(r, g, b)); }
    void SetAlbedo(const Vec3& color) { SetVec3("u_Albedo", color); }
    void SetMetallic(float value) { SetFloat("u_Metallic", value); }
    void SetRoughness(float value) { SetFloat("u_Roughness", value); }
    void SetEmission(const Vec3& color) { SetVec3("u_Emission", color); }
    void SetEmissionIntensity(float value) { SetFloat("u_EmissionIntensity", value); }
    void SetAlphaCutoff(float value) { SetFloat("u_AlphaCutoff", value); }

    // ========================================================================
    // Property Getters
    // ========================================================================

    bool HasProperty(const std::string& name) const;
    MaterialPropertyType GetPropertyType(const std::string& name) const;

    // Type-safe getters (return default if not found or wrong type)
    int GetInt(const std::string& name, int defaultValue = 0) const;
    float GetFloat(const std::string& name, float defaultValue = 0.0f) const;
    Vec2 GetVec2(const std::string& name, const Vec2& defaultValue = Vec2(0.0f)) const;
    Vec3 GetVec3(const std::string& name, const Vec3& defaultValue = Vec3(0.0f)) const;
    Vec4 GetVec4(const std::string& name, const Vec4& defaultValue = Vec4(0.0f)) const;
    const TextureSlot* GetTextureSlot(const std::string& name) const;

    // Get all properties (for serialization/debugging)
    const std::unordered_map<std::string, MaterialProperty>& GetProperties() const { return m_properties; }

    // ========================================================================
    // Render State
    // ========================================================================

    void SetRenderQueue(RenderQueue queue) { m_renderQueue = queue; }
    RenderQueue GetRenderQueue() const { return m_renderQueue; }

    void SetBlendMode(BlendMode mode) { m_blendMode = mode; }
    BlendMode GetBlendMode() const { return m_blendMode; }

    void SetCullMode(CullMode mode) { m_cullMode = mode; }
    CullMode GetCullMode() const { return m_cullMode; }

    void SetDepthWrite(bool enabled) { m_depthWrite = enabled; }
    bool GetDepthWrite() const { return m_depthWrite; }

    void SetDepthTest(bool enabled) { m_depthTest = enabled; }
    bool GetDepthTest() const { return m_depthTest; }

    // ========================================================================
    // Binding - Apply material to current render state
    // ========================================================================

    // Bind shader and upload all properties
    void Bind() const;

    // Unbind (optional, resets to default state)
    void Unbind() const;

    // Apply only render state (blend mode, culling, etc.)
    void ApplyRenderState() const;

    // Upload only properties to currently bound shader
    // (useful when sharing shader between materials)
    void UploadProperties() const;

    // Upload a specific property
    void UploadProperty(const std::string& name) const;

    // ========================================================================
    // Metadata
    // ========================================================================

    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    // Tagging for material organization
    void AddTag(const std::string& tag) { m_tags.push_back(tag); }
    bool HasTag(const std::string& tag) const;
    const std::vector<std::string>& GetTags() const { return m_tags; }

    // ========================================================================
    // Instancing / Cloning
    // ========================================================================

    // Create a copy of this material (same shader, copied properties)
    std::shared_ptr<Material> Clone(const std::string& newName = "") const;

    // Create an instance that shares the shader but has its own properties
    std::shared_ptr<Material> CreateInstance(const std::string& instanceName = "") const;

    // ========================================================================
    // Debug
    // ========================================================================

    void PrintDebugInfo() const;

private:
    std::string m_name;
    std::shared_ptr<Shader> m_shader;

    // Material properties (uniform values)
    std::unordered_map<std::string, MaterialProperty> m_properties;

    // Render state
    RenderQueue m_renderQueue = RenderQueue::Geometry;
    BlendMode m_blendMode = BlendMode::Opaque;
    CullMode m_cullMode = CullMode::Back;
    bool m_depthWrite = true;
    bool m_depthTest = true;

    // Tags for organization
    std::vector<std::string> m_tags;

    // Instance counter for auto-naming
    static int s_instanceCounter;
};

// ============================================================================
// Shared pointer alias
// ============================================================================
using MaterialPtr = std::shared_ptr<Material>;

} // namespace Genesis

