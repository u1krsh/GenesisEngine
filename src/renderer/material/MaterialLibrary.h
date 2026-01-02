#pragma once

#include "Material.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

namespace Genesis {

// ============================================================================
// Material Template - Predefined material configurations
// ============================================================================
struct MaterialTemplate {
    std::string name;
    std::string shaderName;
    std::unordered_map<std::string, MaterialPropertyValue> defaultProperties;
    RenderQueue renderQueue = RenderQueue::Geometry;
    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
};

// ============================================================================
// MaterialLibrary - Manages material loading, caching, and creation
//
// Follows the Source engine pattern where materials are loaded from files
// or created programmatically, cached by name, and can be looked up quickly.
// ============================================================================
class MaterialLibrary {
public:
    static MaterialLibrary& Instance() {
        static MaterialLibrary instance;
        return instance;
    }

    // ========================================================================
    // Material Management
    // ========================================================================

    // Create a new material with the given name and shader
    MaterialPtr Create(const std::string& name, const std::string& shaderName);
    MaterialPtr Create(const std::string& name, std::shared_ptr<Shader> shader);

    // Create material from template
    MaterialPtr CreateFromTemplate(const std::string& name, const std::string& templateName);

    // Get a previously created material
    MaterialPtr Get(const std::string& name) const;

    // Check if material exists
    bool Exists(const std::string& name) const;

    // Remove a material from the library
    void Remove(const std::string& name);

    // Clear all materials
    void Clear();

    // Get all material names
    std::vector<std::string> GetMaterialNames() const;

    // Get materials by tag
    std::vector<MaterialPtr> GetMaterialsByTag(const std::string& tag) const;

    // Get materials by shader
    std::vector<MaterialPtr> GetMaterialsByShader(const std::string& shaderName) const;

    // ========================================================================
    // Template Management
    // ========================================================================

    // Register a material template
    void RegisterTemplate(const MaterialTemplate& tmpl);

    // Get template by name
    const MaterialTemplate* GetTemplate(const std::string& name) const;

    // Check if template exists
    bool TemplateExists(const std::string& name) const;

    // Register built-in templates
    void RegisterBuiltInTemplates();

    // ========================================================================
    // Quick Creation Helpers
    // ========================================================================

    // Create a simple solid color material
    MaterialPtr CreateSolidColor(const std::string& name, const Vec3& color);
    MaterialPtr CreateSolidColor(const std::string& name, float r, float g, float b);

    // Create an unlit material
    MaterialPtr CreateUnlit(const std::string& name, const Vec3& color);

    // Create a transparent material
    MaterialPtr CreateTransparent(const std::string& name, const Vec4& color);

    // Create a material with emission
    MaterialPtr CreateEmissive(const std::string& name, const Vec3& color,
                                const Vec3& emission, float intensity = 1.0f);

    // ========================================================================
    // Batch Operations
    // ========================================================================

    // Reload all shaders used by materials
    void ReloadShaders();

    // Iterate over all materials
    void ForEach(const std::function<void(const std::string&, MaterialPtr)>& callback) const;

    // ========================================================================
    // File Loading (Future: load from .mat files)
    // ========================================================================

    // Set base path for material files
    void SetMaterialBasePath(const std::string& path) { m_basePath = path; }
    const std::string& GetMaterialBasePath() const { return m_basePath; }

    // Load material from file (placeholder for future .mat file support)
    // MaterialPtr LoadFromFile(const std::string& path);

    // ========================================================================
    // Debug
    // ========================================================================

    void PrintDebugInfo() const;

private:
    MaterialLibrary();
    ~MaterialLibrary() = default;
    MaterialLibrary(const MaterialLibrary&) = delete;
    MaterialLibrary& operator=(const MaterialLibrary&) = delete;

private:
    std::unordered_map<std::string, MaterialPtr> m_materials;
    std::unordered_map<std::string, MaterialTemplate> m_templates;
    std::string m_basePath = "assets/materials/";
};

} // namespace Genesis

