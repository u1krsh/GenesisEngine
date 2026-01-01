#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <chrono>
#include <vector>
#include "camera/Camera.h"  // For Vec3, Mat4

namespace Genesis {

// ============================================================================
// Uniform Types - Type-safe uniform value storage
// ============================================================================
enum class UniformType {
    Unknown,
    Int,
    Float,
    Vec2,
    Vec3,
    Vec4,
    Mat3,
    Mat4,
    Sampler2D
};

struct Vec2 {
    float x = 0.0f, y = 0.0f;
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}
};

struct Vec4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    Vec4() = default;
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

struct Mat3 {
    float m[9] = {
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
    };
    const float* Data() const { return m; }
};

// ============================================================================
// Uniform Info - Cached uniform metadata
// ============================================================================
struct UniformInfo {
    std::string name;
    int location = -1;
    UniformType type = UniformType::Unknown;
    int size = 1;  // Array size (1 for non-arrays)
};

// ============================================================================
// Shader - Represents a compiled and linked GPU shader program
// ============================================================================
class Shader {
public:
    Shader();
    ~Shader();

    // Non-copyable, movable
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // ========================================================================
    // Loading
    // ========================================================================

    // Load shader from vertex and fragment file paths
    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);

    // Load shader from source strings
    bool LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource,
                        const std::string& debugName = "inline");

    // Reload shader from files (for hot reloading)
    bool Reload();

    // Check if files have been modified since last load
    bool NeedsReload() const;

    // ========================================================================
    // Usage
    // ========================================================================

    void Bind() const;
    void Unbind() const;
    bool IsValid() const { return m_programId != 0; }

    // ========================================================================
    // Type-Safe Uniform Setters
    // ========================================================================

    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVec2(const std::string& name, const Vec2& value);
    void SetVec2(const std::string& name, float x, float y);
    void SetVec3(const std::string& name, const Vec3& value);
    void SetVec3(const std::string& name, float x, float y, float z);
    void SetVec4(const std::string& name, const Vec4& value);
    void SetVec4(const std::string& name, float x, float y, float z, float w);
    void SetMat3(const std::string& name, const Mat3& value, bool transpose = false);
    void SetMat4(const std::string& name, const Mat4& value, bool transpose = false);
    void SetSampler(const std::string& name, int textureUnit);

    // ========================================================================
    // Introspection
    // ========================================================================

    unsigned int GetProgramId() const { return m_programId; }
    const std::string& GetName() const { return m_name; }
    const std::string& GetVertexPath() const { return m_vertexPath; }
    const std::string& GetFragmentPath() const { return m_fragmentPath; }

    // Get all cached uniforms
    const std::unordered_map<std::string, UniformInfo>& GetUniforms() const { return m_uniformCache; }

    // Check if uniform exists
    bool HasUniform(const std::string& name) const;

    // Get uniform location (cached)
    int GetUniformLocation(const std::string& name) const;

    // Print shader info for debugging
    void PrintDebugInfo() const;

private:
    // Compilation helpers
    unsigned int CompileShader(unsigned int type, const std::string& source);
    bool LinkProgram(unsigned int vertexShader, unsigned int fragmentShader);
    void CacheUniforms();
    void Cleanup();

    // File helpers
    static std::string ReadFile(const std::string& path);
    static std::filesystem::file_time_type GetFileModTime(const std::string& path);

private:
    unsigned int m_programId = 0;
    std::string m_name;

    // File paths (for hot reloading)
    std::string m_vertexPath;
    std::string m_fragmentPath;
    std::filesystem::file_time_type m_vertexModTime;
    std::filesystem::file_time_type m_fragmentModTime;

    // Uniform cache: name -> info
    mutable std::unordered_map<std::string, UniformInfo> m_uniformCache;
};

// ============================================================================
// ShaderLibrary - Manages shader loading and caching
// ============================================================================
class ShaderLibrary {
public:
    static ShaderLibrary& Instance() {
        static ShaderLibrary instance;
        return instance;
    }

    // ========================================================================
    // Shader Management
    // ========================================================================

    // Load a shader and store it with a name
    std::shared_ptr<Shader> Load(const std::string& name,
                                   const std::string& vertexPath,
                                   const std::string& fragmentPath);

    // Get a previously loaded shader
    std::shared_ptr<Shader> Get(const std::string& name) const;

    // Check if shader exists
    bool Exists(const std::string& name) const;

    // Remove a shader
    void Remove(const std::string& name);

    // Clear all shaders
    void Clear();

    // ========================================================================
    // Hot Reloading
    // ========================================================================

    // Check all shaders for changes and reload if needed
    void CheckForReloads();

    // Force reload all shaders
    void ReloadAll();

    // Enable/disable hot reloading
    void SetHotReloadEnabled(bool enabled) { m_hotReloadEnabled = enabled; }
    bool IsHotReloadEnabled() const { return m_hotReloadEnabled; }

    // Set hot reload check interval (in seconds)
    void SetHotReloadInterval(float seconds) { m_hotReloadInterval = seconds; }

    // ========================================================================
    // Configuration
    // ========================================================================

    // Set base path for shader files
    void SetShaderBasePath(const std::string& path) { m_basePath = path; }
    const std::string& GetShaderBasePath() const { return m_basePath; }

    // Print all loaded shaders
    void PrintDebugInfo() const;

private:
    ShaderLibrary() = default;
    ~ShaderLibrary() = default;
    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;

private:
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;
    std::string m_basePath = "assets/shaders/";
    bool m_hotReloadEnabled = true;
    float m_hotReloadInterval = 1.0f;  // Check every second
    float m_timeSinceLastCheck = 0.0f;
};

} // namespace Genesis

