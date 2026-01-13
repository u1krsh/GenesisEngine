#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Genesis {
    class Material;
    using MaterialPtr = std::shared_ptr<Material>;
}

// ============================================================================
// MaterialBrowser - ImGui window for browsing and selecting materials/textures
// ============================================================================
class MaterialBrowser {
public:
    static MaterialBrowser& Instance() {
        static MaterialBrowser instance;
        return instance;
    }

    // ========================================================================
    // Lifecycle
    // ========================================================================

    void Initialize();
    void Shutdown();

    // ========================================================================
    // Rendering
    // ========================================================================

    // Render the browser window
    void Render();

    // ========================================================================
    // Selection
    // ========================================================================

    // Get currently selected material name
    const std::string& GetSelectedMaterial() const { return m_selectedMaterial; }

    // Check if a material was just selected this frame
    bool WasMaterialSelected() const { return m_materialJustSelected; }

    // Clear selection state
    void ClearSelection() { m_materialJustSelected = false; }

    // ========================================================================
    // Window Control
    // ========================================================================

    void Open() { m_isOpen = true; }
    void Close() { m_isOpen = false; }
    void Toggle() { m_isOpen = !m_isOpen; }
    bool IsOpen() const { return m_isOpen; }

    // Set callback for when material is applied
    using MaterialApplyCallback = std::function<void(const std::string&)>;
    void SetApplyCallback(MaterialApplyCallback callback) { m_applyCallback = callback; }

private:
    MaterialBrowser() = default;
    ~MaterialBrowser() = default;

    void RenderMaterialGrid();
    void RenderTextureGrid();
    void RenderCategoryFilter();
    void RenderPreview();
    void ScanMaterials();
    void ScanTextures();

private:
    bool m_isOpen = false;
    bool m_initialized = false;
    bool m_materialJustSelected = false;

    // Current selection
    std::string m_selectedMaterial;
    std::string m_selectedTexture;

    // Search/filter
    char m_searchBuffer[256] = "";
    std::string m_currentCategory = "All";
    std::vector<std::string> m_categories;

    // Material/texture lists
    std::vector<std::string> m_materialNames;
    std::vector<std::string> m_texturePaths;

    // Tabs
    int m_currentTab = 0;  // 0 = Materials, 1 = Textures

    // Thumbnail size
    float m_thumbnailSize = 64.0f;

    // Callback
    MaterialApplyCallback m_applyCallback;
};
