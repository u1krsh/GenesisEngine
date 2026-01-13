#include "Texture2D.h"
#include "core/Logger.h"

#include <glad/glad.h>

// stb_image is already implemented in another file (Viewport.cpp)
// Just include the header without STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Genesis {

Texture2D::Texture2D() = default;

Texture2D::Texture2D(const std::string& name) : m_name(name) {}

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
        
        other.m_textureID = 0;
        other.m_thumbnailID = 0;
    }
    return *this;
}

void Texture2D::Destroy() {
    if (m_thumbnailID != 0) {
        glDeleteTextures(1, &m_thumbnailID);
        m_thumbnailID = 0;
    }
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }
    m_width = 0;
    m_height = 0;
    m_channels = 0;
}

bool Texture2D::LoadFromFile(const std::string& filepath) {
    Destroy();
    m_filepath = filepath;

    // Flip vertically for OpenGL (bottom-left origin)
    stbi_set_flip_vertically_on_load(true);

    // Load image data
    unsigned char* data = stbi_load(filepath.c_str(), &m_width, &m_height, &m_channels, 0);
    if (!data) {
        LOG_ERROR("Texture2D", "Failed to load texture: " + filepath + " - " + stbi_failure_reason());
        return false;
    }

    // Determine OpenGL format
    GLenum internalFormat = GL_RGB;
    GLenum dataFormat = GL_RGB;
    
    if (m_channels == 1) {
        internalFormat = GL_R8;
        dataFormat = GL_RED;
    } else if (m_channels == 2) {
        internalFormat = GL_RG8;
        dataFormat = GL_RG;
    } else if (m_channels == 3) {
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    } else if (m_channels == 4) {
        internalFormat = GL_RGBA8;
        dataFormat = GL_RGBA;
    }

    // Create OpenGL texture
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    // Upload data
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, 
                 dataFormat, GL_UNSIGNED_BYTE, data);

    // Free image data
    stbi_image_free(data);

    // Apply settings
    ApplyFilterSettings();
    ApplyWrapSettings();

    // Generate mipmaps by default
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Extract name from filepath if not set
    if (m_name.empty()) {
        size_t lastSlash = filepath.find_last_of("/\\");
        size_t lastDot = filepath.find_last_of('.');
        if (lastSlash != std::string::npos) {
            m_name = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);
        } else {
            m_name = filepath.substr(0, lastDot);
        }
    }

    LOG_INFO("Texture2D", "Loaded texture: " + m_name + " (" + 
             std::to_string(m_width) + "x" + std::to_string(m_height) + 
             ", " + std::to_string(m_channels) + " channels)");

    return true;
}

bool Texture2D::Create(int width, int height, int channels, const unsigned char* data) {
    Destroy();

    m_width = width;
    m_height = height;
    m_channels = channels;

    GLenum internalFormat = GL_RGBA8;
    GLenum dataFormat = GL_RGBA;
    
    if (channels == 1) {
        internalFormat = GL_R8;
        dataFormat = GL_RED;
    } else if (channels == 3) {
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    }

    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, 
                 dataFormat, GL_UNSIGNED_BYTE, data);

    ApplyFilterSettings();
    ApplyWrapSettings();
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

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
    if (m_textureID == 0) return;
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

void Texture2D::ApplyFilterSettings() {
    switch (m_filter) {
        case TextureFilter::Nearest:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        case TextureFilter::Linear:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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
        case TextureWrap::Repeat: wrapMode = GL_REPEAT; break;
        case TextureWrap::Clamp: wrapMode = GL_CLAMP_TO_EDGE; break;
        case TextureWrap::Mirror: wrapMode = GL_MIRRORED_REPEAT; break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
}

unsigned int Texture2D::GetThumbnailID(int maxSize) {
    if (m_textureID == 0) return 0;
    
    // Return cached thumbnail if same size
    if (m_thumbnailID != 0 && m_thumbnailSize == maxSize) {
        return m_thumbnailID;
    }

    // For now, just return the main texture
    // TODO: Generate downscaled thumbnail for very large textures
    return m_textureID;
}

} // namespace Genesis
