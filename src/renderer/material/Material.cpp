#include "Material.h"
#include <glad/glad.h>
#include <iostream>
#include <algorithm>

namespace Genesis {

int Material::s_instanceCounter = 0;

// ============================================================================
// Constructors
// ============================================================================

Material::Material()
    : m_name("Material_" + std::to_string(s_instanceCounter++)) {
}

Material::Material(const std::string& name)
    : m_name(name) {
}

Material::Material(const std::string& name, std::shared_ptr<Shader> shader)
    : m_name(name)
    , m_shader(std::move(shader)) {
}

// ============================================================================
// Shader Management
// ============================================================================

void Material::SetShader(std::shared_ptr<Shader> shader) {
    m_shader = std::move(shader);
}

// ============================================================================
// Property Setters
// ============================================================================

void Material::SetInt(const std::string& name, int value) {
    m_properties[name] = MaterialProperty(name, value);
}

void Material::SetFloat(const std::string& name, float value) {
    m_properties[name] = MaterialProperty(name, value);
}

void Material::SetVec2(const std::string& name, const Vec2& value) {
    m_properties[name] = MaterialProperty(name, value);
}

void Material::SetVec3(const std::string& name, const Vec3& value) {
    m_properties[name] = MaterialProperty(name, value);
}

void Material::SetVec4(const std::string& name, const Vec4& value) {
    m_properties[name] = MaterialProperty(name, value);
}

void Material::SetMat3(const std::string& name, const Mat3& value) {
    m_properties[name] = MaterialProperty(name, value);
}

void Material::SetMat4(const std::string& name, const Mat4& value) {
    m_properties[name] = MaterialProperty(name, value);
}

void Material::SetTexture(const std::string& name, std::shared_ptr<Texture2D> texture, int unit) {
    TextureSlot slot;
    slot.texture = std::move(texture);
    slot.unit = unit;
    slot.uniformName = name;
    m_properties[name] = MaterialProperty(name, slot);
}

void Material::SetTextureSlot(const std::string& name, const TextureSlot& slot) {
    m_properties[name] = MaterialProperty(name, slot);
}

// ============================================================================
// Property Getters
// ============================================================================

bool Material::HasProperty(const std::string& name) const {
    return m_properties.find(name) != m_properties.end();
}

MaterialPropertyType Material::GetPropertyType(const std::string& name) const {
    auto it = m_properties.find(name);
    if (it != m_properties.end()) {
        return it->second.type;
    }
    return MaterialPropertyType::None;
}

int Material::GetInt(const std::string& name, int defaultValue) const {
    auto it = m_properties.find(name);
    if (it != m_properties.end() && it->second.type == MaterialPropertyType::Int) {
        return std::get<int>(it->second.value);
    }
    return defaultValue;
}

float Material::GetFloat(const std::string& name, float defaultValue) const {
    auto it = m_properties.find(name);
    if (it != m_properties.end() && it->second.type == MaterialPropertyType::Float) {
        return std::get<float>(it->second.value);
    }
    return defaultValue;
}

Vec2 Material::GetVec2(const std::string& name, const Vec2& defaultValue) const {
    auto it = m_properties.find(name);
    if (it != m_properties.end() && it->second.type == MaterialPropertyType::Vec2) {
        return std::get<Vec2>(it->second.value);
    }
    return defaultValue;
}

Vec3 Material::GetVec3(const std::string& name, const Vec3& defaultValue) const {
    auto it = m_properties.find(name);
    if (it != m_properties.end() && it->second.type == MaterialPropertyType::Vec3) {
        return std::get<Vec3>(it->second.value);
    }
    return defaultValue;
}

Vec4 Material::GetVec4(const std::string& name, const Vec4& defaultValue) const {
    auto it = m_properties.find(name);
    if (it != m_properties.end() && it->second.type == MaterialPropertyType::Vec4) {
        return std::get<Vec4>(it->second.value);
    }
    return defaultValue;
}

const TextureSlot* Material::GetTextureSlot(const std::string& name) const {
    auto it = m_properties.find(name);
    if (it != m_properties.end() && it->second.type == MaterialPropertyType::Texture2D) {
        return &std::get<TextureSlot>(it->second.value);
    }
    return nullptr;
}

// ============================================================================
// Tag Management
// ============================================================================

bool Material::HasTag(const std::string& tag) const {
    return std::find(m_tags.begin(), m_tags.end(), tag) != m_tags.end();
}

// ============================================================================
// Binding and State Application
// ============================================================================

void Material::Bind() const {
    if (!m_shader || !m_shader->IsValid()) {
        std::cerr << "[Material] Cannot bind material '" << m_name
                  << "': no valid shader" << std::endl;
        return;
    }

    // Bind shader
    m_shader->Bind();

    // Apply render state
    ApplyRenderState();

    // Upload all properties
    UploadProperties();
}

void Material::Unbind() const {
    if (m_shader) {
        m_shader->Unbind();
    }

    // Reset render state to defaults
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void Material::ApplyRenderState() const {
    // Blend mode
    switch (m_blendMode) {
        case BlendMode::Opaque:
            glDisable(GL_BLEND);
            break;
        case BlendMode::Cutout:
            glDisable(GL_BLEND);
            // Alpha cutoff is handled in shader via u_AlphaCutoff
            break;
        case BlendMode::Transparent:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::Additive:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case BlendMode::Multiply:
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
    }

    // Cull mode
    switch (m_cullMode) {
        case CullMode::Off:
            glDisable(GL_CULL_FACE);
            break;
        case CullMode::Back:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case CullMode::Front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
    }

    // Depth write
    glDepthMask(m_depthWrite ? GL_TRUE : GL_FALSE);

    // Depth test
    if (m_depthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void Material::UploadProperties() const {
    if (!m_shader) return;

    for (const auto& [name, prop] : m_properties) {
        UploadProperty(name);
    }
}

void Material::UploadProperty(const std::string& name) const {
    if (!m_shader) return;

    auto it = m_properties.find(name);
    if (it == m_properties.end()) return;

    const MaterialProperty& prop = it->second;

    std::visit([this, &name](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
            // Do nothing
        }
        else if constexpr (std::is_same_v<T, int>) {
            m_shader->SetInt(name, arg);
        }
        else if constexpr (std::is_same_v<T, float>) {
            m_shader->SetFloat(name, arg);
        }
        else if constexpr (std::is_same_v<T, Vec2>) {
            m_shader->SetVec2(name, arg);
        }
        else if constexpr (std::is_same_v<T, Vec3>) {
            m_shader->SetVec3(name, arg);
        }
        else if constexpr (std::is_same_v<T, Vec4>) {
            m_shader->SetVec4(name, arg);
        }
        else if constexpr (std::is_same_v<T, Mat3>) {
            m_shader->SetMat3(name, arg);
        }
        else if constexpr (std::is_same_v<T, Mat4>) {
            m_shader->SetMat4(name, arg);
        }
        else if constexpr (std::is_same_v<T, TextureSlot>) {
            // Bind texture to its unit
            // Note: Texture2D binding would go here when texture system is implemented
            // For now, just set the sampler uniform to the correct unit
            m_shader->SetSampler(name, arg.unit);

            // If we have tiling/offset, upload those too
            // Convention: u_TextureName_ST for scale/tiling and offset
            if (arg.tiling != Vec2(1.0f, 1.0f) || arg.offset != Vec2(0.0f, 0.0f)) {
                std::string stName = name + "_ST";
                m_shader->SetVec4(stName, Vec4(arg.tiling.x, arg.tiling.y,
                                                arg.offset.x, arg.offset.y));
            }
        }
    }, prop.value);
}

// ============================================================================
// Cloning and Instancing
// ============================================================================

std::shared_ptr<Material> Material::Clone(const std::string& newName) const {
    std::string cloneName = newName.empty() ? m_name + "_Clone" : newName;
    auto clone = std::make_shared<Material>(cloneName);

    clone->m_shader = m_shader;
    clone->m_properties = m_properties;
    clone->m_renderQueue = m_renderQueue;
    clone->m_blendMode = m_blendMode;
    clone->m_cullMode = m_cullMode;
    clone->m_depthWrite = m_depthWrite;
    clone->m_depthTest = m_depthTest;
    clone->m_tags = m_tags;

    return clone;
}

std::shared_ptr<Material> Material::CreateInstance(const std::string& instanceName) const {
    std::string instName = instanceName.empty()
        ? m_name + "_Instance_" + std::to_string(s_instanceCounter++)
        : instanceName;

    // Instance shares shader, copies properties
    return Clone(instName);
}

// ============================================================================
// Debug
// ============================================================================

void Material::PrintDebugInfo() const {
    std::cout << "=== Material Debug Info ===" << std::endl;
    std::cout << "Name: " << m_name << std::endl;
    std::cout << "Shader: " << (m_shader ? m_shader->GetName() : "none") << std::endl;
    std::cout << "Render Queue: " << static_cast<int>(m_renderQueue) << std::endl;
    std::cout << "Blend Mode: " << static_cast<int>(m_blendMode) << std::endl;
    std::cout << "Properties (" << m_properties.size() << "):" << std::endl;

    for (const auto& [name, prop] : m_properties) {
        std::cout << "  - " << name << " (";
        switch (prop.type) {
            case MaterialPropertyType::Int: std::cout << "int"; break;
            case MaterialPropertyType::Float: std::cout << "float"; break;
            case MaterialPropertyType::Vec2: std::cout << "vec2"; break;
            case MaterialPropertyType::Vec3: std::cout << "vec3"; break;
            case MaterialPropertyType::Vec4: std::cout << "vec4"; break;
            case MaterialPropertyType::Mat3: std::cout << "mat3"; break;
            case MaterialPropertyType::Mat4: std::cout << "mat4"; break;
            case MaterialPropertyType::Texture2D: std::cout << "texture2D"; break;
            default: std::cout << "unknown"; break;
        }
        std::cout << ")" << std::endl;
    }

    if (!m_tags.empty()) {
        std::cout << "Tags: ";
        for (size_t i = 0; i < m_tags.size(); ++i) {
            std::cout << m_tags[i];
            if (i < m_tags.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << "=========================" << std::endl;
}

} // namespace Genesis

