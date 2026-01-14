#include "TextureLibrary.h"
#include "core/Logger.h"

#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace Genesis {

TextureLibrary::TextureLibrary() {
    LOG_INFO("TextureLibrary", "Initialized");
}

Texture2DPtr TextureLibrary::Load(const std::string& filepath) {
    // Early return for empty paths
    if (filepath.empty()) {
        LOG_WARNING("TextureLibrary", "Attempted to load empty filepath");
        return nullptr;
    }
    
    std::string normalizedPath = NormalizePath(filepath);
    
    // Check cache first
    auto it = m_textures.find(normalizedPath);
    if (it != m_textures.end()) {
        return it->second;
    }

    // Try to load
    auto texture = std::make_shared<Texture2D>();
    
    // Build list of paths to try
    std::vector<std::string> pathsToTry;
    
    // Try exact path first
    pathsToTry.push_back(filepath);
    
    // Try with base path (assets/textures/)
    if (filepath[0] != '/') {
        pathsToTry.push_back(m_basePath + filepath);
    }
    
    // Try loading from each path
    std::string loadPath;
    bool loaded = false;
    for (const auto& path : pathsToTry) {
        if (fs::exists(path)) {
            loadPath = path;
            if (texture->LoadFromFile(loadPath)) {
                loaded = true;
                break;
            }
        }
    }

    if (!loaded) {
        LOG_ERROR("TextureLibrary", "Failed to load texture: " + filepath + 
                  " (tried: " + m_basePath + filepath + ")");
        return nullptr;
    }

    // Cache it and log success
    m_textures[normalizedPath] = texture;
    LOG_DEBUG("TextureLibrary", "Loaded texture: " + loadPath);
    return texture;
}

Texture2DPtr TextureLibrary::Get(const std::string& name) const {
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        return it->second;
    }
    return nullptr;
}

bool TextureLibrary::Exists(const std::string& name) const {
    return m_textures.find(name) != m_textures.end();
}

void TextureLibrary::Unload(const std::string& name) {
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        m_textures.erase(it);
        LOG_INFO("TextureLibrary", "Unloaded texture: " + name);
    }
}

void TextureLibrary::Clear() {
    m_textures.clear();
    LOG_INFO("TextureLibrary", "Cleared all textures");
}

void TextureLibrary::ScanDirectory(const std::string& path) {
    m_discoveredPaths.clear();
    m_lastScanPath = path;

    std::string scanPath = path;
    if (path.empty()) {
        scanPath = m_basePath;
    }

    if (!fs::exists(scanPath)) {
        LOG_WARNING("TextureLibrary", "Texture directory does not exist: " + scanPath);
        return;
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(scanPath)) {
            if (entry.is_regular_file() && IsTextureFile(entry.path().string())) {
                m_discoveredPaths.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("TextureLibrary", "Error scanning directory: " + std::string(e.what()));
    }

    std::sort(m_discoveredPaths.begin(), m_discoveredPaths.end());
    LOG_INFO("TextureLibrary", "Discovered " + std::to_string(m_discoveredPaths.size()) + 
             " textures in " + scanPath);
}

void TextureLibrary::RefreshDiscovery() {
    if (!m_lastScanPath.empty()) {
        ScanDirectory(m_lastScanPath);
    } else {
        ScanDirectory(m_basePath);
    }
}

Texture2DPtr TextureLibrary::CreateCheckerboard(const std::string& name, int size,
                                                  const Vec4& color1, const Vec4& color2) {
    std::vector<unsigned char> pixels(size * size * 4);
    
    int checkSize = size / 8;  // 8x8 checks
    if (checkSize < 1) checkSize = 1;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            bool check = ((x / checkSize) + (y / checkSize)) % 2 == 0;
            const Vec4& color = check ? color1 : color2;
            
            int idx = (y * size + x) * 4;
            pixels[idx + 0] = static_cast<unsigned char>(color.r * 255.0f);
            pixels[idx + 1] = static_cast<unsigned char>(color.g * 255.0f);
            pixels[idx + 2] = static_cast<unsigned char>(color.b * 255.0f);
            pixels[idx + 3] = static_cast<unsigned char>(color.a * 255.0f);
        }
    }

    auto texture = std::make_shared<Texture2D>(name);
    texture->Create(size, size, 4, pixels.data());
    m_textures[name] = texture;
    
    LOG_INFO("TextureLibrary", "Created checkerboard texture: " + name);
    return texture;
}

Texture2DPtr TextureLibrary::CreateSolid(const std::string& name, int size, const Vec4& color) {
    auto texture = std::make_shared<Texture2D>(name);
    texture->CreateSolidColor(size, size, color);
    m_textures[name] = texture;
    
    LOG_INFO("TextureLibrary", "Created solid texture: " + name);
    return texture;
}

Texture2DPtr TextureLibrary::CreateGrid(const std::string& name, int size,
                                         const Vec4& bgColor, const Vec4& lineColor) {
    std::vector<unsigned char> pixels(size * size * 4);
    
    int gridSpacing = size / 8;  // 8 grid lines
    if (gridSpacing < 4) gridSpacing = 4;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            bool isLine = (x % gridSpacing == 0) || (y % gridSpacing == 0);
            const Vec4& color = isLine ? lineColor : bgColor;
            
            int idx = (y * size + x) * 4;
            pixels[idx + 0] = static_cast<unsigned char>(color.r * 255.0f);
            pixels[idx + 1] = static_cast<unsigned char>(color.g * 255.0f);
            pixels[idx + 2] = static_cast<unsigned char>(color.b * 255.0f);
            pixels[idx + 3] = static_cast<unsigned char>(color.a * 255.0f);
        }
    }

    auto texture = std::make_shared<Texture2D>(name);
    texture->Create(size, size, 4, pixels.data());
    m_textures[name] = texture;
    
    LOG_INFO("TextureLibrary", "Created grid texture: " + name);
    return texture;
}

std::vector<std::string> TextureLibrary::GetLoadedTextureNames() const {
    std::vector<std::string> names;
    names.reserve(m_textures.size());
    for (const auto& [name, tex] : m_textures) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

bool TextureLibrary::IsTextureFile(const std::string& path) const {
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || 
           ext == ".tga" || ext == ".bmp" || ext == ".gif";
}

std::string TextureLibrary::NormalizePath(const std::string& path) const {
    std::string result = path;
    
    // Replace backslashes with forward slashes
    std::replace(result.begin(), result.end(), '\\', '/');
    
    // Remove trailing slash
    if (!result.empty() && result.back() == '/') {
        result.pop_back();
    }
    
    return result;
}

} // namespace Genesis
