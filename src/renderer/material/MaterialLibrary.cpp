#include "MaterialLibrary.h"
#include "renderer/shader/Shader.h"
#include "core/Logger.h"
#include <iostream>

namespace Genesis {

// ============================================================================
// Constructor
// ============================================================================

MaterialLibrary::MaterialLibrary() {
    // Register built-in templates on construction
    RegisterBuiltInTemplates();
}

// ============================================================================
// Material Management
// ============================================================================

MaterialPtr MaterialLibrary::Create(const std::string& name, const std::string& shaderName) {
    // Check if material already exists
    if (Exists(name)) {
        LOG_WARNING("MaterialLibrary", "Material '" + name + "' already exists, returning existing");
        return Get(name);
    }

    // Get shader from library
    auto shader = ShaderLibrary::Instance().Get(shaderName);
    if (!shader) {
        LOG_ERROR("MaterialLibrary", "Shader '" + shaderName + "' not found for material '" + name + "'");
        return nullptr;
    }

    return Create(name, shader);
}

MaterialPtr MaterialLibrary::Create(const std::string& name, std::shared_ptr<Shader> shader) {
    // Check if material already exists
    if (Exists(name)) {
        LOG_WARNING("MaterialLibrary", "Material '" + name + "' already exists, returning existing");
        return Get(name);
    }

    auto material = std::make_shared<Material>(name, shader);
    m_materials[name] = material;

    LOG_INFO("MaterialLibrary", "Created material '" + name + "'");
    return material;
}

MaterialPtr MaterialLibrary::CreateFromTemplate(const std::string& name, const std::string& templateName) {
    const MaterialTemplate* tmpl = GetTemplate(templateName);
    if (!tmpl) {
        LOG_ERROR("MaterialLibrary", "Template '" + templateName + "' not found");
        return nullptr;
    }

    // Create material with template's shader
    auto material = Create(name, tmpl->shaderName);
    if (!material) {
        return nullptr;
    }

    // Apply template settings
    material->SetRenderQueue(tmpl->renderQueue);
    material->SetBlendMode(tmpl->blendMode);
    material->SetCullMode(tmpl->cullMode);

    // Apply default properties
    for (const auto& [propName, propValue] : tmpl->defaultProperties) {
        std::visit([&material, &propName](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int>) {
                material->SetInt(propName, arg);
            } else if constexpr (std::is_same_v<T, float>) {
                material->SetFloat(propName, arg);
            } else if constexpr (std::is_same_v<T, Vec2>) {
                material->SetVec2(propName, arg);
            } else if constexpr (std::is_same_v<T, Vec3>) {
                material->SetVec3(propName, arg);
            } else if constexpr (std::is_same_v<T, Vec4>) {
                material->SetVec4(propName, arg);
            } else if constexpr (std::is_same_v<T, TextureSlot>) {
                material->SetTextureSlot(propName, arg);
            }
        }, propValue);
    }

    return material;
}

MaterialPtr MaterialLibrary::Get(const std::string& name) const {
    auto it = m_materials.find(name);
    if (it != m_materials.end()) {
        return it->second;
    }
    return nullptr;
}

bool MaterialLibrary::Exists(const std::string& name) const {
    return m_materials.find(name) != m_materials.end();
}

void MaterialLibrary::Remove(const std::string& name) {
    auto it = m_materials.find(name);
    if (it != m_materials.end()) {
        m_materials.erase(it);
        LOG_INFO("MaterialLibrary", "Removed material '" + name + "'");
    }
}

void MaterialLibrary::Clear() {
    m_materials.clear();
    LOG_INFO("MaterialLibrary", "Cleared all materials");
}

std::vector<std::string> MaterialLibrary::GetMaterialNames() const {
    std::vector<std::string> names;
    names.reserve(m_materials.size());
    for (const auto& [name, mat] : m_materials) {
        names.push_back(name);
    }
    return names;
}

std::vector<MaterialPtr> MaterialLibrary::GetMaterialsByTag(const std::string& tag) const {
    std::vector<MaterialPtr> result;
    for (const auto& [name, mat] : m_materials) {
        if (mat->HasTag(tag)) {
            result.push_back(mat);
        }
    }
    return result;
}

std::vector<MaterialPtr> MaterialLibrary::GetMaterialsByShader(const std::string& shaderName) const {
    std::vector<MaterialPtr> result;
    for (const auto& [name, mat] : m_materials) {
        auto shader = mat->GetShader();
        if (shader && shader->GetName() == shaderName) {
            result.push_back(mat);
        }
    }
    return result;
}

// ============================================================================
// Template Management
// ============================================================================

void MaterialLibrary::RegisterTemplate(const MaterialTemplate& tmpl) {
    m_templates[tmpl.name] = tmpl;
    LOG_INFO("MaterialLibrary", "Registered template '" + tmpl.name + "'");
}

const MaterialTemplate* MaterialLibrary::GetTemplate(const std::string& name) const {
    auto it = m_templates.find(name);
    if (it != m_templates.end()) {
        return &it->second;
    }
    return nullptr;
}

bool MaterialLibrary::TemplateExists(const std::string& name) const {
    return m_templates.find(name) != m_templates.end();
}

void MaterialLibrary::RegisterBuiltInTemplates() {
    // LitOpaque - Standard lit opaque material (like Source's LightmappedGeneric)
    {
        MaterialTemplate tmpl;
        tmpl.name = "LitOpaque";
        tmpl.shaderName = "mesh";
        tmpl.renderQueue = RenderQueue::Geometry;
        tmpl.blendMode = BlendMode::Opaque;
        tmpl.cullMode = CullMode::Back;
        tmpl.defaultProperties["u_Color"] = Vec3(1.0f, 1.0f, 1.0f);
        RegisterTemplate(tmpl);
    }

    // Unlit - Unlit material (like Source's UnlitGeneric)
    {
        MaterialTemplate tmpl;
        tmpl.name = "Unlit";
        tmpl.shaderName = "basic";
        tmpl.renderQueue = RenderQueue::Geometry;
        tmpl.blendMode = BlendMode::Opaque;
        tmpl.cullMode = CullMode::Back;
        tmpl.defaultProperties["u_Color"] = Vec3(1.0f, 1.0f, 1.0f);
        RegisterTemplate(tmpl);
    }

    // Transparent - Alpha blended material
    {
        MaterialTemplate tmpl;
        tmpl.name = "Transparent";
        tmpl.shaderName = "mesh";
        tmpl.renderQueue = RenderQueue::Transparent;
        tmpl.blendMode = BlendMode::Transparent;
        tmpl.cullMode = CullMode::Back;
        tmpl.defaultProperties["u_Color"] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        RegisterTemplate(tmpl);
    }

    // Debug - Debug visualization material
    {
        MaterialTemplate tmpl;
        tmpl.name = "Debug";
        tmpl.shaderName = "debug";
        tmpl.renderQueue = RenderQueue::Geometry;
        tmpl.blendMode = BlendMode::Opaque;
        tmpl.cullMode = CullMode::Off;
        RegisterTemplate(tmpl);
    }
}

// ============================================================================
// Quick Creation Helpers
// ============================================================================

MaterialPtr MaterialLibrary::CreateSolidColor(const std::string& name, const Vec3& color) {
    auto material = CreateFromTemplate(name, "LitOpaque");
    if (material) {
        material->SetColor(color);
    }
    return material;
}

MaterialPtr MaterialLibrary::CreateSolidColor(const std::string& name, float r, float g, float b) {
    return CreateSolidColor(name, Vec3(r, g, b));
}

MaterialPtr MaterialLibrary::CreateUnlit(const std::string& name, const Vec3& color) {
    auto material = CreateFromTemplate(name, "Unlit");
    if (material) {
        material->SetColor(color);
    }
    return material;
}

MaterialPtr MaterialLibrary::CreateTransparent(const std::string& name, const Vec4& color) {
    auto material = CreateFromTemplate(name, "Transparent");
    if (material) {
        material->SetColor(color);
        material->SetDepthWrite(false);  // Common for transparent objects
    }
    return material;
}

MaterialPtr MaterialLibrary::CreateEmissive(const std::string& name, const Vec3& color,
                                             const Vec3& emission, float intensity) {
    auto material = CreateFromTemplate(name, "LitOpaque");
    if (material) {
        material->SetColor(color);
        material->SetEmission(emission);
        material->SetEmissionIntensity(intensity);
    }
    return material;
}

// ============================================================================
// Batch Operations
// ============================================================================

void MaterialLibrary::ReloadShaders() {
    ShaderLibrary::Instance().ReloadAll();
}

void MaterialLibrary::ForEach(const std::function<void(const std::string&, MaterialPtr)>& callback) const {
    for (const auto& [name, mat] : m_materials) {
        callback(name, mat);
    }
}

// ============================================================================
// Debug
// ============================================================================

void MaterialLibrary::PrintDebugInfo() const {
    std::cout << "=== MaterialLibrary Debug Info ===" << std::endl;
    std::cout << "Materials: " << m_materials.size() << std::endl;

    for (const auto& [name, mat] : m_materials) {
        std::cout << "  - " << name;
        if (mat->GetShader()) {
            std::cout << " (shader: " << mat->GetShader()->GetName() << ")";
        }
        std::cout << std::endl;
    }

    std::cout << "Templates: " << m_templates.size() << std::endl;
    for (const auto& [name, tmpl] : m_templates) {
        std::cout << "  - " << name << " (shader: " << tmpl.shaderName << ")" << std::endl;
    }
    std::cout << "=================================" << std::endl;
}

} // namespace Genesis

