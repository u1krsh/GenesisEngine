#pragma once

// ============================================================================
// OracularV2 Texture Browser
// Hammer-style texture selection dialog with grid view and preview
// ============================================================================

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <glad/glad.h>
#include <imgui.h>
#include "stb_image.h"

namespace fs = std::filesystem;

// ============================================================================
// Texture Category - Groups textures by type
// ============================================================================
struct TextureCategory {
    std::string name;           // Display name
    std::string path;           // Relative path from assets/textures
    std::vector<std::string> textures;  // List of texture filenames
};

// ============================================================================
// TextureThumbnail - Cached texture preview
// ============================================================================
struct TextureThumbnail {
    GLuint textureId = 0;
    int width = 0;
    int height = 0;
    bool loaded = false;
};

// ============================================================================
// TextureBrowser - Modal texture selection dialog
// ============================================================================
class TextureBrowser {
public:
    TextureBrowser();
    ~TextureBrowser();
    
    // ========================================================================
    // Main API
    // ========================================================================
    
    // Open the browser as a modal popup
    void Open(const std::string& currentTexture = "");
    
    // Render the browser window (call each frame when open)
    // Returns true if a texture was selected
    bool Render();
    
    // Close the browser
    void Close();
    
    // Check if browser is open
    bool IsOpen() const { return m_isOpen; }
    
    // Get the selected texture path (relative to assets/textures)
    const std::string& GetSelectedTexture() const { return m_selectedTexture; }
    
    // Set callback for when texture is selected
    void SetSelectCallback(std::function<void(const std::string&)> callback) {
        m_onSelect = callback;
    }
    
    // ========================================================================
    // Configuration
    // ========================================================================
    
    void SetTexturesPath(const std::string& path) { m_texturesPath = path; }
    void RefreshCategories();  // Rescan texture directories
    
private:
    // ========================================================================
    // Internal Methods
    // ========================================================================
    
    void ScanTextureDirectories();
    void LoadThumbnail(const std::string& texturePath);
    void RenderCategoryFilter();
    void RenderTextureGrid();
    void RenderPreviewPanel();
    void ClearThumbnails();
    
    // Lazy thumbnail loading
    GLuint GetOrLoadThumbnail(const std::string& texturePath);
    
private:
    bool m_isOpen = false;
    std::string m_texturesPath = "assets/textures";
    
    // Selection state
    std::string m_selectedTexture;
    std::string m_hoveredTexture;
    std::string m_previewTexture;
    
    // Filter state
    std::string m_filterText;
    std::string m_selectedCategory = "All";
    int m_thumbnailSize = 128;
    
    // Texture categories
    std::vector<TextureCategory> m_categories;
    std::vector<std::string> m_filteredTextures;
    
    // Thumbnail cache
    std::unordered_map<std::string, TextureThumbnail> m_thumbnailCache;
    size_t m_maxCacheSize = 200;  // Max thumbnails to keep in memory
    
    // Callback
    std::function<void(const std::string&)> m_onSelect;
};

// ============================================================================
// Implementation
// ============================================================================

inline TextureBrowser::TextureBrowser() {
    RefreshCategories();
}

inline TextureBrowser::~TextureBrowser() {
    ClearThumbnails();
}

inline void TextureBrowser::Open(const std::string& currentTexture) {
    m_isOpen = true;
    m_selectedTexture = currentTexture;
    m_previewTexture = currentTexture;
    m_filterText.clear();
    RefreshCategories();
    ImGui::OpenPopup("Texture Browser");
}

inline void TextureBrowser::Close() {
    m_isOpen = false;
}

inline void TextureBrowser::RefreshCategories() {
    ScanTextureDirectories();
}

inline void TextureBrowser::ScanTextureDirectories() {
    m_categories.clear();
    
    // Add "All" category
    TextureCategory allCategory;
    allCategory.name = "All";
    allCategory.path = "";
    
    try {
        fs::path basePath(m_texturesPath);
        if (!fs::exists(basePath)) return;
        
        for (const auto& entry : fs::directory_iterator(basePath)) {
            if (entry.is_directory()) {
                TextureCategory cat;
                cat.name = entry.path().filename().string();
                cat.path = cat.name;
                
                // Scan textures in this directory
                for (const auto& texEntry : fs::directory_iterator(entry.path())) {
                    if (texEntry.is_regular_file()) {
                        std::string ext = texEntry.path().extension().string();
                        // Only include image files, exclude normal/mask maps from main list
                        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga") {
                            std::string filename = texEntry.path().filename().string();
                            // Skip auxiliary maps (normal, mask, height)
                            if (filename.find("_normal") == std::string::npos &&
                                filename.find("_mask") == std::string::npos &&
                                filename.find("_height") == std::string::npos) {
                                std::string relPath = cat.path + "/" + filename;
                                cat.textures.push_back(relPath);
                                allCategory.textures.push_back(relPath);
                            }
                        }
                    }
                }
                
                if (!cat.textures.empty()) {
                    m_categories.push_back(cat);
                }
            }
        }
    } catch (...) {
        // Ignore filesystem errors
    }
    
    // Insert "All" at the beginning
    m_categories.insert(m_categories.begin(), allCategory);
    
    // Initialize filtered list
    if (!m_categories.empty()) {
        m_filteredTextures = m_categories[0].textures;
    }
}

inline bool TextureBrowser::Render() {
    bool textureSelected = false;
    
    if (!m_isOpen) return false;
    
    // Set popup size
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_Appearing);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    
    if (ImGui::BeginPopupModal("Texture Browser", &m_isOpen, flags)) {
        // Top bar: Category filter and search
        ImGui::BeginChild("TopBar", ImVec2(0, 35), false);
        {
            // Category dropdown
            ImGui::SetNextItemWidth(150);
            if (ImGui::BeginCombo("##Category", m_selectedCategory.c_str())) {
                for (const auto& cat : m_categories) {
                    bool isSelected = (m_selectedCategory == cat.name);
                    if (ImGui::Selectable(cat.name.c_str(), isSelected)) {
                        m_selectedCategory = cat.name;
                        // Update filtered list
                        for (const auto& c : m_categories) {
                            if (c.name == m_selectedCategory) {
                                m_filteredTextures = c.textures;
                                break;
                            }
                        }
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            
            ImGui::SameLine();
            
            // Search filter
            char filterBuf[128];
            strncpy(filterBuf, m_filterText.c_str(), sizeof(filterBuf) - 1);
            filterBuf[sizeof(filterBuf) - 1] = '\0';
            ImGui::SetNextItemWidth(200);
            if (ImGui::InputTextWithHint("##Filter", "Search textures...", filterBuf, sizeof(filterBuf))) {
                m_filterText = filterBuf;
            }
            
            ImGui::SameLine();
            
            // Thumbnail size
            ImGui::Text("Size:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            const char* sizes[] = {"64", "96", "128", "192"};
            int sizeIdx = 2; // Default 128
            if (m_thumbnailSize == 64) sizeIdx = 0;
            else if (m_thumbnailSize == 96) sizeIdx = 1;
            else if (m_thumbnailSize == 128) sizeIdx = 2;
            else if (m_thumbnailSize == 192) sizeIdx = 3;
            
            if (ImGui::Combo("##ThumbnailSize", &sizeIdx, sizes, IM_ARRAYSIZE(sizes))) {
                int vals[] = {64, 96, 128, 192};
                m_thumbnailSize = vals[sizeIdx];
            }
            
            ImGui::SameLine();
            ImGui::Text("(%zu textures)", m_filteredTextures.size());
        }
        ImGui::EndChild();
        
        ImGui::Separator();
        
        // Main content: Texture grid + Preview panel
        float previewWidth = 200.0f;
        
        // Texture grid (left side)
        ImGui::BeginChild("TextureGrid", ImVec2(-previewWidth - 10, -40), true, ImGuiWindowFlags_HorizontalScrollbar);
        {
            float windowWidth = ImGui::GetContentRegionAvail().x;
            int columns = std::max(1, (int)(windowWidth / (m_thumbnailSize + 10)));
            int col = 0;
            
            for (const auto& texPath : m_filteredTextures) {
                // Apply text filter
                if (!m_filterText.empty()) {
                    std::string lowerPath = texPath;
                    std::string lowerFilter = m_filterText;
                    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
                    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
                    if (lowerPath.find(lowerFilter) == std::string::npos) {
                        continue;
                    }
                }
                
                if (col > 0) ImGui::SameLine();
                
                ImGui::BeginGroup();
                {
                    // Get or load thumbnail
                    GLuint texId = GetOrLoadThumbnail(m_texturesPath + "/" + texPath);
                    
                    // Highlight selected
                    bool isSelected = (texPath == m_selectedTexture);
                    if (isSelected) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.9f, 1.0f));
                    }
                    
                    // Texture button
                    ImGui::PushID(texPath.c_str());
                    if (texId != 0) {
                        if (ImGui::ImageButton("##tex", (ImTextureID)(intptr_t)texId, 
                                              ImVec2((float)m_thumbnailSize, (float)m_thumbnailSize))) {
                            m_selectedTexture = texPath;
                            m_previewTexture = texPath;
                        }
                    } else {
                        // Placeholder for loading textures
                        if (ImGui::Button("?", ImVec2((float)m_thumbnailSize, (float)m_thumbnailSize))) {
                            m_selectedTexture = texPath;
                            m_previewTexture = texPath;
                        }
                    }
                    
                    // Double-click to select and close
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        textureSelected = true;
                        if (m_onSelect) m_onSelect(texPath);
                        Close();
                    }
                    
                    // Hover preview
                    if (ImGui::IsItemHovered()) {
                        m_previewTexture = texPath;
                    }
                    
                    ImGui::PopID();
                    
                    if (isSelected) {
                        ImGui::PopStyleColor();
                    }
                    
                    // Texture name (truncated)
                    std::string filename = fs::path(texPath).stem().string();
                    if (filename.length() > 14) {
                        filename = filename.substr(0, 11) + "...";
                    }
                    ImGui::TextWrapped("%s", filename.c_str());
                }
                ImGui::EndGroup();
                
                col++;
                if (col >= columns) col = 0;
            }
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Preview panel (right side)
        ImGui::BeginChild("Preview", ImVec2(previewWidth, -40), true);
        {
            ImGui::Text("Preview");
            ImGui::Separator();
            
            if (!m_previewTexture.empty()) {
                GLuint texId = GetOrLoadThumbnail(m_texturesPath + "/" + m_previewTexture);
                if (texId != 0) {
                    float previewSize = previewWidth - 20;
                    ImGui::Image((ImTextureID)(intptr_t)texId, ImVec2(previewSize, previewSize));
                }
                
                ImGui::Separator();
                ImGui::TextWrapped("%s", m_previewTexture.c_str());
                
                // Check for associated maps
                std::string baseName = fs::path(m_previewTexture).stem().string();
                std::string dir = fs::path(m_previewTexture).parent_path().string();
                
                ImGui::Spacing();
                ImGui::Text("Associated maps:");
                
                // Check for normal map
                std::string normalPath = dir + "/" + baseName + "_normal.png";
                if (fs::exists(m_texturesPath + "/" + normalPath)) {
                    ImGui::BulletText("Normal map");
                }
                
                // Check for mask
                std::string maskPath = dir + "/" + baseName + "_mask.png";
                if (fs::exists(m_texturesPath + "/" + maskPath)) {
                    ImGui::BulletText("Mask map");
                }
            }
        }
        ImGui::EndChild();
        
        ImGui::Separator();
        
        // Bottom buttons
        if (ImGui::Button("Select", ImVec2(100, 0))) {
            textureSelected = true;
            if (m_onSelect) m_onSelect(m_selectedTexture);
            Close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            Close();
        }
        
        ImGui::EndPopup();
    }
    
    return textureSelected;
}

inline GLuint TextureBrowser::GetOrLoadThumbnail(const std::string& texturePath) {
    // Check cache
    auto it = m_thumbnailCache.find(texturePath);
    if (it != m_thumbnailCache.end() && it->second.loaded) {
        return it->second.textureId;
    }
    
    // Load texture using stb_image (assuming it's available)
    // For now, return 0 to indicate loading needed
    // This should ideally be done in a separate thread
    
    // Simple synchronous load for now
    LoadThumbnail(texturePath);
    
    it = m_thumbnailCache.find(texturePath);
    if (it != m_thumbnailCache.end()) {
        return it->second.textureId;
    }
    
    return 0;
}

inline void TextureBrowser::LoadThumbnail(const std::string& texturePath) {
    // Skip if already loaded
    if (m_thumbnailCache.count(texturePath) && m_thumbnailCache[texturePath].loaded) {
        return;
    }
    
    // Load image using stb_image
    stbi_set_flip_vertically_on_load(true);
    
    int width, height, channels;
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        // Mark as failed
        TextureThumbnail thumb;
        thumb.loaded = false;
        thumb.textureId = 0;
        m_thumbnailCache[texturePath] = thumb;
        return;
    }
    
    // Create OpenGL texture
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    stbi_image_free(data);
    
    // Cache the thumbnail
    TextureThumbnail thumb;
    thumb.textureId = texId;
    thumb.width = width;
    thumb.height = height;
    thumb.loaded = true;
    m_thumbnailCache[texturePath] = thumb;
}

inline void TextureBrowser::ClearThumbnails() {
    for (auto& pair : m_thumbnailCache) {
        if (pair.second.textureId != 0) {
            glDeleteTextures(1, &pair.second.textureId);
        }
    }
    m_thumbnailCache.clear();
}
