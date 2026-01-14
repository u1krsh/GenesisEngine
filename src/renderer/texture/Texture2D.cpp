#include "Texture2D.h"
#include "core/Logger.h"
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Genesis {

Texture2D::Texture2D() 
    : m_name("Unnamed")
{
}

Texture2D::Texture2D(const std::string& name) 
    : m_name(name)
{
}

Texture2D::~Texture2D() {
    Destroy();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_name(std::move(other.m_name))
    , m_filepath(std::move(other.m_filepath))
    , m_textureID(other.m_textureID)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_channels(other.m_channels)
    , m_filter(other.m_filter)
    , m_wrap(other.m_wrap)
    , m_thumbnailID(other.m_thumbnailID)
    , m_thumbnailSize(other.m_thumbnailSize)
    , m_pixelData(std::move(other.m_pixelData))
{
    other.m_textureID = 0;
    other.m_thumbnailID = 0;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        Destroy();
        m_name = std::move(other.m_name);
        m_filepath = std::move(other.m_filepath);
        m_textureID = other.m_textureID;
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;
        m_filter = other.m_filter;
        m_wrap = other.m_wrap;
        m_thumbnailID = other.m_thumbnailID;
        m_thumbnailSize = other.m_thumbnailSize;
        m_pixelData = std::move(other.m_pixelData);
        
        other.m_textureID = 0;
        other.m_thumbnailID = 0;
    }
    return *this;
}

bool Texture2D::LoadFromFile(const std::string& filepath) {
    // Flip vertically for OpenGL (bottom-left origin)
    stbi_set_flip_vertically_on_load(true);
    
    int width, height, channels;
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        LOG_ERROR("Texture2D", "Failed to load texture: " + filepath + " - " + stbi_failure_reason());
        return false;
    }
    
    bool success = Create(width, height, channels, data);
    stbi_image_free(data);
    
    if (success) {
        m_filepath = filepath;
        if (m_name == "Unnamed") {
            // Extract filename from path
            size_t lastSlash = filepath.find_last_of("/\\");
            m_name = (lastSlash != std::string::npos) ? filepath.substr(lastSlash + 1) : filepath;
        }
        LOG_INFO("Texture2D", "Loaded texture: " + filepath + " (" + 
                 std::to_string(width) + "x" + std::to_string(height) + ", " + 
                 std::to_string(channels) + " channels)");
    }
    
    return success;
}

bool Texture2D::Create(int width, int height, int channels, const unsigned char* data) {
    Destroy();
    
    m_width = width;
    m_height = height;
    m_channels = channels;
    
    GLenum format = GL_RGBA;
    GLenum internalFormat = GL_RGBA8;
    
    switch (channels) {
        case 1:
            format = GL_RED;
            internalFormat = GL_R8;
            break;
        case 2:
            format = GL_RG;
            internalFormat = GL_RG8;
            break;
        case 3:
            format = GL_RGB;
            internalFormat = GL_RGB8;
            break;
        case 4:
        default:
            format = GL_RGBA;
            internalFormat = GL_RGBA8;
            break;
    }
    
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    ApplyFilterSettings();
    ApplyWrapSettings();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Store pixel data CPU-side for alpha sampling (used in light baking)
    if (data && channels >= 3) {
        m_pixelData.assign(data, data + (width * height * channels));
    }
    
    return true;
}

bool Texture2D::CreateSolidColor(int width, int height, const Vec4& color) {
    std::vector<unsigned char> pixels(width * height * 4);
    
    unsigned char r = static_cast<unsigned char>(color.r * 255.0f);
    unsigned char g = static_cast<unsigned char>(color.g * 255.0f);
    unsigned char b = static_cast<unsigned char>(color.b * 255.0f);
    unsigned char a = static_cast<unsigned char>(color.a * 255.0f);
    
    for (int i = 0; i < width * height; ++i) {
        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = a;
    }
    
    return Create(width, height, 4, pixels.data());
}

void Texture2D::Bind(int textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void Texture2D::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::SetFilter(TextureFilter filter) {
    m_filter = filter;
    if (m_textureID != 0) {
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        ApplyFilterSettings();
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Texture2D::SetWrap(TextureWrap wrap) {
    m_wrap = wrap;
    if (m_textureID != 0) {
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        ApplyWrapSettings();
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Texture2D::GenerateMipmaps() {
    if (m_textureID != 0) {
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

unsigned int Texture2D::GetThumbnailID(int maxSize) {
    // For simplicity, just return the main texture ID
    // A proper implementation would create a downscaled version
    return m_textureID;
}

void Texture2D::Destroy() {
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }
    if (m_thumbnailID != 0) {
        glDeleteTextures(1, &m_thumbnailID);
        m_thumbnailID = 0;
    }
    m_pixelData.clear();
}

void Texture2D::ApplyFilterSettings() {
    switch (m_filter) {
        case TextureFilter::Nearest:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        case TextureFilter::Linear:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        case TextureFilter::Trilinear:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
    }
}

void Texture2D::ApplyWrapSettings() {
    GLenum wrapMode = GL_REPEAT;
    switch (m_wrap) {
        case TextureWrap::Repeat:
            wrapMode = GL_REPEAT;
            break;
        case TextureWrap::Clamp:
            wrapMode = GL_CLAMP_TO_EDGE;
            break;
        case TextureWrap::Mirror:
            wrapMode = GL_MIRRORED_REPEAT;
            break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
}

float Texture2D::SampleAlpha(float u, float v) const {
    // Return 1.0 (opaque) if no pixel data or not enough channels
    if (m_pixelData.empty() || m_channels < 4 || m_width <= 0 || m_height <= 0) {
        return 1.0f;
    }
    
    // Wrap UVs to [0, 1)
    u = u - std::floor(u);
    v = v - std::floor(v);
    
    // Convert to pixel coordinates
    int x = static_cast<int>(u * m_width) % m_width;
    int y = static_cast<int>(v * m_height) % m_height;
    
    // Get pixel index (alpha is 4th channel)
    size_t idx = (static_cast<size_t>(y) * m_width + x) * m_channels + 3;
    
    if (idx < m_pixelData.size()) {
        return m_pixelData[idx] / 255.0f;
    }
    
    return 1.0f;
}

} // namespace Genesis
