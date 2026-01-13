#include "MaterialBrowser.h"

#include "renderer/material/MaterialLibrary.h"
#include "renderer/texture/TextureLibrary.h"
#include "core/Logger.h"

#include <imgui.h>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
using namespace Genesis;

void MaterialBrowser::Initialize() {
    if (m_initialized) return;
    
    ScanMaterials();
    ScanTextures();
    
    m_initialized = true;
    LOG_INFO("MaterialBrowser", "Initialized");
}

void MaterialBrowser::Shutdown() {
    m_initialized = false;
}

void MaterialBrowser::ScanMaterials() {
    m_materialNames.clear();
    m_materialNames = MaterialLibrary::Instance().GetMaterialNames();
    
    // Add built-in material types
    if (std::find(m_materialNames.begin(), m_materialNames.end(), "glass") == m_materialNames.end()) {
        m_materialNames.push_back("glass");
    }
    
    std::sort(m_materialNames.begin(), m_materialNames.end());
}

void MaterialBrowser::ScanTextures() {
    m_texturePaths.clear();
    m_categories.clear();
    m_categories.push_back("All");
    
    auto& texLib = TextureLibrary::Instance();
    
    // Scan the textures directory
    std::string texturePath = "assets/textures/";
    
#ifdef ASSETS_DIR
    texturePath = std::string(ASSETS_DIR) + "/textures/";
#endif
    
    texLib.ScanDirectory(texturePath);
    m_texturePaths = texLib.GetDiscoveredTextures();
    
    // Extract categories from paths
    for (const auto& path : m_texturePaths) {
        fs::path p(path);
        if (p.has_parent_path()) {
            std::string cat = p.parent_path().filename().string();
            if (!cat.empty() && std::find(m_categories.begin(), m_categories.end(), cat) == m_categories.end()) {
                m_categories.push_back(cat);
            }
        }
    }
    
    std::sort(m_categories.begin() + 1, m_categories.end());  // Keep "All" first
}

void MaterialBrowser::Render() {
    if (!m_isOpen) return;
    
    m_materialJustSelected = false;
    
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Material Browser", &m_isOpen)) {
        // Tab bar
        if (ImGui::BeginTabBar("MaterialBrowserTabs")) {
            if (ImGui::BeginTabItem("Materials")) {
                m_currentTab = 0;
                RenderMaterialGrid();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Textures")) {
                m_currentTab = 1;
                RenderCategoryFilter();
                ImGui::Separator();
                RenderTextureGrid();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void MaterialBrowser::RenderCategoryFilter() {
    ImGui::Text("Category:");
    ImGui::SameLine();
    
    if (ImGui::BeginCombo("##Category", m_currentCategory.c_str())) {
        for (const auto& cat : m_categories) {
            bool selected = (m_currentCategory == cat);
            if (ImGui::Selectable(cat.c_str(), selected)) {
                m_currentCategory = cat;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::SameLine();
    ImGui::Text("Search:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##Search", m_searchBuffer, sizeof(m_searchBuffer));
}

void MaterialBrowser::RenderMaterialGrid() {
    // Refresh button
    if (ImGui::Button("Refresh")) {
        ScanMaterials();
    }
    ImGui::SameLine();
    ImGui::Text("%zu materials", m_materialNames.size());
    
    ImGui::Separator();
    
    // Material grid
    float windowWidth = ImGui::GetContentRegionAvail().x;
    int columns = std::max(1, static_cast<int>(windowWidth / (m_thumbnailSize + 20)));
    
    if (ImGui::BeginChild("MaterialGrid", ImVec2(0, -30), true)) {
        int col = 0;
        
        for (const auto& matName : m_materialNames) {
            // Filter by search
            if (m_searchBuffer[0] != '\0') {
                std::string lower = matName;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                std::string search = m_searchBuffer;
                std::transform(search.begin(), search.end(), search.begin(), ::tolower);
                if (lower.find(search) == std::string::npos) {
                    continue;
                }
            }
            
            ImGui::PushID(matName.c_str());
            
            bool selected = (m_selectedMaterial == matName);
            
            // Draw colored box as material preview
            ImVec2 size(m_thumbnailSize, m_thumbnailSize);
            ImVec4 color(0.4f, 0.4f, 0.5f, 1.0f);
            
            // Color hint based on material name
            if (matName.find("glass") != std::string::npos) {
                color = ImVec4(0.6f, 0.8f, 0.9f, 0.5f);
            } else if (matName.find("brick") != std::string::npos) {
                color = ImVec4(0.6f, 0.3f, 0.2f, 1.0f);
            } else if (matName.find("metal") != std::string::npos) {
                color = ImVec4(0.5f, 0.5f, 0.6f, 1.0f);
            } else if (matName.find("stone") != std::string::npos) {
                color = ImVec4(0.35f, 0.35f, 0.38f, 1.0f);
            } else if (matName.find("wood") != std::string::npos) {
                color = ImVec4(0.5f, 0.35f, 0.2f, 1.0f);
            } else if (matName.find("floor") != std::string::npos) {
                color = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
            } else if (matName.find("wall") != std::string::npos) {
                color = ImVec4(0.5f, 0.5f, 0.52f, 1.0f);
            } else if (matName.find("concrete") != std::string::npos) {
                color = ImVec4(0.55f, 0.55f, 0.57f, 1.0f);
            }
            
            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.3f, 0.7f, 1.0f));
            }
            
            if (ImGui::ColorButton(("##Mat" + matName).c_str(), color, 
                                   ImGuiColorEditFlags_NoTooltip, size)) {
                m_selectedMaterial = matName;
                m_materialJustSelected = true;
            }
            
            if (selected) {
                ImGui::PopStyleColor();
            }
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", matName.c_str());
            }
            
            // Label below
            ImGui::TextWrapped("%s", matName.c_str());
            
            ImGui::PopID();
            
            col++;
            if (col < columns) {
                ImGui::SameLine();
            } else {
                col = 0;
            }
        }
    }
    ImGui::EndChild();
    
    // Apply button
    ImGui::BeginDisabled(m_selectedMaterial.empty());
    if (ImGui::Button("Apply to Selection")) {
        if (m_applyCallback) {
            m_applyCallback(m_selectedMaterial);
        }
    }
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    ImGui::Text("Selected: %s", m_selectedMaterial.empty() ? "(none)" : m_selectedMaterial.c_str());
}

void MaterialBrowser::RenderTextureGrid() {
    float windowWidth = ImGui::GetContentRegionAvail().x;
    int columns = std::max(1, static_cast<int>(windowWidth / (m_thumbnailSize + 20)));
    
    if (ImGui::BeginChild("TextureGrid", ImVec2(0, -30), true)) {
        int col = 0;
        
        for (const auto& texPath : m_texturePaths) {
            // Category filter
            if (m_currentCategory != "All") {
                fs::path p(texPath);
                std::string cat = p.parent_path().filename().string();
                if (cat != m_currentCategory) {
                    continue;
                }
            }
            
            // Search filter
            if (m_searchBuffer[0] != '\0') {
                std::string lower = texPath;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                std::string search = m_searchBuffer;
                std::transform(search.begin(), search.end(), search.begin(), ::tolower);
                if (lower.find(search) == std::string::npos) {
                    continue;
                }
            }
            
            ImGui::PushID(texPath.c_str());
            
            bool selected = (m_selectedTexture == texPath);
            
            // Try to get texture thumbnail
            auto& texLib = TextureLibrary::Instance();
            auto tex = texLib.Load(texPath);
            
            ImVec2 size(m_thumbnailSize, m_thumbnailSize);
            
            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.3f, 0.7f, 1.0f));
            }
            
            if (tex && tex->IsLoaded()) {
                // Use actual texture
                if (ImGui::ImageButton(("##Tex" + texPath).c_str(),
                                       (ImTextureID)(intptr_t)tex->GetID(),
                                       size)) {
                    m_selectedTexture = texPath;
                    m_materialJustSelected = true;
                }
            } else {
                // Placeholder
                if (ImGui::Button("?", size)) {
                    m_selectedTexture = texPath;
                    m_materialJustSelected = true;
                }
            }
            
            if (selected) {
                ImGui::PopStyleColor();
            }
            
            if (ImGui::IsItemHovered()) {
                fs::path p(texPath);
                ImGui::SetTooltip("%s", p.filename().string().c_str());
            }
            
            // Filename below
            fs::path p(texPath);
            std::string filename = p.filename().string();
            if (filename.length() > 10) {
                filename = filename.substr(0, 8) + "..";
            }
            ImGui::TextWrapped("%s", filename.c_str());
            
            ImGui::PopID();
            
            col++;
            if (col < columns) {
                ImGui::SameLine();
            } else {
                col = 0;
            }
        }
    }
    ImGui::EndChild();
    
    // Status bar
    ImGui::Text("%zu textures", m_texturePaths.size());
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        ScanTextures();
    }
}

void MaterialBrowser::RenderPreview() {
    // TODO: Render 3D preview of selected material on a sphere
}
