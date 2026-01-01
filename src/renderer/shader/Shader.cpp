#include "Shader.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace Genesis {

// ============================================================================
// Shader Implementation
// ============================================================================

Shader::Shader() = default;

Shader::~Shader() {
    Cleanup();
}

Shader::Shader(Shader&& other) noexcept
    : m_programId(other.m_programId)
    , m_name(std::move(other.m_name))
    , m_vertexPath(std::move(other.m_vertexPath))
    , m_fragmentPath(std::move(other.m_fragmentPath))
    , m_vertexModTime(other.m_vertexModTime)
    , m_fragmentModTime(other.m_fragmentModTime)
    , m_uniformCache(std::move(other.m_uniformCache))
{
    other.m_programId = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_programId = other.m_programId;
        m_name = std::move(other.m_name);
        m_vertexPath = std::move(other.m_vertexPath);
        m_fragmentPath = std::move(other.m_fragmentPath);
        m_vertexModTime = other.m_vertexModTime;
        m_fragmentModTime = other.m_fragmentModTime;
        m_uniformCache = std::move(other.m_uniformCache);
        other.m_programId = 0;
    }
    return *this;
}

bool Shader::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    // Store paths for hot reloading
    m_vertexPath = vertexPath;
    m_fragmentPath = fragmentPath;

    // Extract name from path
    std::filesystem::path vp(vertexPath);
    m_name = vp.stem().string();

    // Read shader sources
    std::string vertexSource = ReadFile(vertexPath);
    std::string fragmentSource = ReadFile(fragmentPath);

    if (vertexSource.empty()) {
        std::cerr << "[Shader] Failed to read vertex shader: " << vertexPath << std::endl;
        return false;
    }
    if (fragmentSource.empty()) {
        std::cerr << "[Shader] Failed to read fragment shader: " << fragmentPath << std::endl;
        return false;
    }

    // Store modification times
    m_vertexModTime = GetFileModTime(vertexPath);
    m_fragmentModTime = GetFileModTime(fragmentPath);

    return LoadFromSource(vertexSource, fragmentSource, m_name);
}

bool Shader::LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource,
                             const std::string& debugName) {
    // Cleanup any existing program
    Cleanup();

    if (m_name.empty()) {
        m_name = debugName;
    }

    // Compile shaders
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) {
        return false;
    }

    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    // Link program
    if (!LinkProgram(vertexShader, fragmentShader)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Shaders are linked, can delete them
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Cache uniform locations
    CacheUniforms();

    std::cout << "[Shader] Loaded shader '" << m_name << "' (ID: " << m_programId
              << ", Uniforms: " << m_uniformCache.size() << ")" << std::endl;

    return true;
}

bool Shader::Reload() {
    if (m_vertexPath.empty() || m_fragmentPath.empty()) {
        std::cerr << "[Shader] Cannot reload shader '" << m_name << "': no file paths stored" << std::endl;
        return false;
    }

    std::cout << "[Shader] Reloading shader '" << m_name << "'..." << std::endl;

    // Store old program ID in case reload fails
    unsigned int oldProgram = m_programId;
    auto oldCache = m_uniformCache;
    m_programId = 0;
    m_uniformCache.clear();

    // Try to reload
    if (!LoadFromFiles(m_vertexPath, m_fragmentPath)) {
        // Restore old program
        m_programId = oldProgram;
        m_uniformCache = oldCache;
        std::cerr << "[Shader] Failed to reload shader '" << m_name << "', keeping old version" << std::endl;
        return false;
    }

    // Delete old program (new one was created successfully)
    if (oldProgram != 0) {
        glDeleteProgram(oldProgram);
    }

    std::cout << "[Shader] Successfully reloaded shader '" << m_name << "'" << std::endl;
    return true;
}

bool Shader::NeedsReload() const {
    if (m_vertexPath.empty() || m_fragmentPath.empty()) {
        return false;
    }

    try {
        auto currentVertexMod = GetFileModTime(m_vertexPath);
        auto currentFragmentMod = GetFileModTime(m_fragmentPath);

        return currentVertexMod != m_vertexModTime || currentFragmentMod != m_fragmentModTime;
    } catch (...) {
        return false;
    }
}

void Shader::Bind() const {
    if (m_programId != 0) {
        glUseProgram(m_programId);
    }
}

void Shader::Unbind() const {
    glUseProgram(0);
}

// ============================================================================
// Uniform Setters
// ============================================================================

void Shader::SetInt(const std::string& name, int value) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void Shader::SetFloat(const std::string& name, float value) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void Shader::SetVec2(const std::string& name, const Vec2& value) {
    SetVec2(name, value.x, value.y);
}

void Shader::SetVec2(const std::string& name, float x, float y) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        glUniform2f(location, x, y);
    }
}

void Shader::SetVec3(const std::string& name, const Vec3& value) {
    SetVec3(name, value.x, value.y, value.z);
}

void Shader::SetVec3(const std::string& name, float x, float y, float z) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

void Shader::SetVec4(const std::string& name, const Vec4& value) {
    SetVec4(name, value.x, value.y, value.z, value.w);
}

void Shader::SetVec4(const std::string& name, float x, float y, float z, float w) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        glUniform4f(location, x, y, z, w);
    }
}

void Shader::SetMat3(const std::string& name, const Mat3& value, bool transpose) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        glUniformMatrix3fv(location, 1, transpose ? GL_TRUE : GL_FALSE, value.Data());
    }
}

void Shader::SetMat4(const std::string& name, const Mat4& value, bool transpose) {
    int location = GetUniformLocation(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, transpose ? GL_TRUE : GL_FALSE, value.Data());
    }
}

void Shader::SetSampler(const std::string& name, int textureUnit) {
    SetInt(name, textureUnit);
}

// ============================================================================
// Introspection
// ============================================================================

bool Shader::HasUniform(const std::string& name) const {
    return m_uniformCache.find(name) != m_uniformCache.end();
}

int Shader::GetUniformLocation(const std::string& name) const {
    // Check cache first
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) {
        return it->second.location;
    }

    // Not in cache, query OpenGL and cache it
    if (m_programId == 0) return -1;

    int location = glGetUniformLocation(m_programId, name.c_str());

    // Cache even if -1 to avoid repeated lookups
    UniformInfo info;
    info.name = name;
    info.location = location;
    info.type = UniformType::Unknown;
    m_uniformCache[name] = info;

    if (location == -1) {
        // Only warn once per uniform
        static std::unordered_map<std::string, bool> warnedUniforms;
        std::string key = m_name + ":" + name;
        if (warnedUniforms.find(key) == warnedUniforms.end()) {
            std::cerr << "[Shader] Warning: uniform '" << name << "' not found in shader '"
                      << m_name << "'" << std::endl;
            warnedUniforms[key] = true;
        }
    }

    return location;
}

void Shader::PrintDebugInfo() const {
    std::cout << "=== Shader Debug Info ===" << std::endl;
    std::cout << "Name: " << m_name << std::endl;
    std::cout << "Program ID: " << m_programId << std::endl;
    std::cout << "Vertex Path: " << m_vertexPath << std::endl;
    std::cout << "Fragment Path: " << m_fragmentPath << std::endl;
    std::cout << "Uniforms (" << m_uniformCache.size() << "):" << std::endl;
    for (const auto& [name, info] : m_uniformCache) {
        std::cout << "  - " << name << " (location: " << info.location << ")" << std::endl;
    }
    std::cout << "=========================" << std::endl;
}

// ============================================================================
// Private Helpers
// ============================================================================

unsigned int Shader::CompileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Check compilation status
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);

        std::string shaderType = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        std::cerr << "[Shader] " << shaderType << " shader compilation failed in '"
                  << m_name << "':\n" << infoLog << std::endl;

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool Shader::LinkProgram(unsigned int vertexShader, unsigned int fragmentShader) {
    m_programId = glCreateProgram();
    glAttachShader(m_programId, vertexShader);
    glAttachShader(m_programId, fragmentShader);
    glLinkProgram(m_programId);

    // Check linking status
    int success;
    glGetProgramiv(m_programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(m_programId, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "[Shader] Program linking failed for '" << m_name << "':\n"
                  << infoLog << std::endl;

        glDeleteProgram(m_programId);
        m_programId = 0;
        return false;
    }

    return true;
}

void Shader::CacheUniforms() {
    m_uniformCache.clear();

    if (m_programId == 0) return;

    int uniformCount;
    glGetProgramiv(m_programId, GL_ACTIVE_UNIFORMS, &uniformCount);

    char nameBuffer[256];
    for (int i = 0; i < uniformCount; i++) {
        int size;
        unsigned int type;
        glGetActiveUniform(m_programId, i, sizeof(nameBuffer), nullptr, &size, &type, nameBuffer);

        UniformInfo info;
        info.name = nameBuffer;
        info.location = glGetUniformLocation(m_programId, nameBuffer);
        info.size = size;

        // Map OpenGL type to our enum
        switch (type) {
            case GL_INT:
            case GL_BOOL:
                info.type = UniformType::Int;
                break;
            case GL_FLOAT:
                info.type = UniformType::Float;
                break;
            case GL_FLOAT_VEC2:
                info.type = UniformType::Vec2;
                break;
            case GL_FLOAT_VEC3:
                info.type = UniformType::Vec3;
                break;
            case GL_FLOAT_VEC4:
                info.type = UniformType::Vec4;
                break;
            case GL_FLOAT_MAT3:
                info.type = UniformType::Mat3;
                break;
            case GL_FLOAT_MAT4:
                info.type = UniformType::Mat4;
                break;
            case GL_SAMPLER_2D:
                info.type = UniformType::Sampler2D;
                break;
            default:
                info.type = UniformType::Unknown;
                break;
        }

        m_uniformCache[info.name] = info;
    }
}

void Shader::Cleanup() {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
        m_programId = 0;
    }
    m_uniformCache.clear();
}

std::string Shader::ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Shader] Failed to open file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::filesystem::file_time_type Shader::GetFileModTime(const std::string& path) {
    try {
        return std::filesystem::last_write_time(path);
    } catch (const std::exception&) {
        std::cerr << "[Shader] Failed to get modification time for: " << path << std::endl;
        return std::filesystem::file_time_type{};
    }
}

// ============================================================================
// ShaderLibrary Implementation
// ============================================================================

std::shared_ptr<Shader> ShaderLibrary::Load(const std::string& name,
                                              const std::string& vertexPath,
                                              const std::string& fragmentPath) {
    // Check if already loaded
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        std::cout << "[ShaderLibrary] Shader '" << name << "' already loaded, returning cached version" << std::endl;
        return it->second;
    }

    // Create and load new shader
    auto shader = std::make_shared<Shader>();

    std::string fullVertexPath = m_basePath + vertexPath;
    std::string fullFragmentPath = m_basePath + fragmentPath;

    if (!shader->LoadFromFiles(fullVertexPath, fullFragmentPath)) {
        std::cerr << "[ShaderLibrary] Failed to load shader '" << name << "'" << std::endl;
        return nullptr;
    }

    m_shaders[name] = shader;
    return shader;
}

std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name) const {
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return it->second;
    }

    std::cerr << "[ShaderLibrary] Shader '" << name << "' not found" << std::endl;
    return nullptr;
}

bool ShaderLibrary::Exists(const std::string& name) const {
    return m_shaders.find(name) != m_shaders.end();
}

void ShaderLibrary::Remove(const std::string& name) {
    m_shaders.erase(name);
}

void ShaderLibrary::Clear() {
    m_shaders.clear();
}

void ShaderLibrary::CheckForReloads() {
    if (!m_hotReloadEnabled) return;

    for (auto& [name, shader] : m_shaders) {
        if (shader && shader->NeedsReload()) {
            shader->Reload();
        }
    }
}

void ShaderLibrary::ReloadAll() {
    std::cout << "[ShaderLibrary] Reloading all shaders..." << std::endl;
    for (auto& [name, shader] : m_shaders) {
        if (shader) {
            shader->Reload();
        }
    }
}

void ShaderLibrary::PrintDebugInfo() const {
    std::cout << "=== ShaderLibrary Debug ===" << std::endl;
    std::cout << "Base Path: " << m_basePath << std::endl;
    std::cout << "Hot Reload: " << (m_hotReloadEnabled ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Loaded Shaders (" << m_shaders.size() << "):" << std::endl;
    for (const auto& [name, shader] : m_shaders) {
        if (shader) {
            std::cout << "  - " << name << " (ID: " << shader->GetProgramId()
                      << ", Valid: " << (shader->IsValid() ? "Yes" : "No") << ")" << std::endl;
        }
    }
    std::cout << "============================" << std::endl;
}

} // namespace Genesis

