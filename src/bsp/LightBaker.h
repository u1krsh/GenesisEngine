#pragma once

#include "BSPTypes.h"
#include "BSPTree.h"
#include "LightmapAtlas.h"
#include "math/Math.h"
#include <vector>
#include <cstdint>
#include <functional>

namespace Genesis {

// ============================================================================
// Light Types for Baking
// ============================================================================
enum class BakeLightType : uint8_t {
    Point,
    Directional,
    Spot
};

// ============================================================================
// Light Source Definition for Baking
// ============================================================================
struct BakeLight {
    Vec3 position = Vec3(0.0f);
    Vec3 direction = Vec3(0.0f, -1.0f, 0.0f);  // For directional/spot
    Vec3 color = Vec3(1.0f);
    float intensity = 1.0f;
    float radius = 10.0f;           // Attenuation radius
    float sourceRadius = 0.5f;      // Size of light source (for soft shadows)
    float spotAngle = 45.0f;        // Spot cone angle (degrees)
    float spotSoftness = 0.1f;      // Soft edge falloff
    BakeLightType type = BakeLightType::Point;
    
    BakeLight() = default;
    
    // Convenience constructors
    static BakeLight CreatePoint(const Vec3& pos, const Vec3& col, float intensity, float radius) {
        BakeLight l;
        l.position = pos;
        l.color = col;
        l.intensity = intensity;
        l.radius = radius;
        l.type = BakeLightType::Point;
        return l;
    }
    
    static BakeLight CreateDirectional(const Vec3& dir, const Vec3& col, float intensity) {
        BakeLight l;
        l.direction = glm::normalize(dir);
        l.color = col;
        l.intensity = intensity;
        l.radius = 1000.0f;  // Far distance for directional
        l.type = BakeLightType::Directional;
        return l;
    }
};

// ============================================================================
// Light Baker - Offline CPU-based Light Baking
// ============================================================================
class LightBaker {
public:
    // Baking options
    struct Options {
        float texelsPerUnit = 4.0f;       // Lightmap resolution (higher = more detail)
        uint32_t minLightmapSize = 4;     // Minimum texels per face
        uint32_t maxLightmapSize = 64;    // Maximum texels per face
        uint32_t numSamples = 1;          // Rays per texel (1 = fast, 4+ = smoother)
        float ambientLight = 0.05f;       // Ambient light level
        float shadowBias = 0.01f;         // Offset for shadow rays
        bool verbose = true;              // Print progress
        
        Options() = default;  // Explicit defaulted constructor
    };
    
    // Progress callback (current face, total faces)
    using ProgressCallback = std::function<void(uint32_t, uint32_t)>;
    
    LightBaker() = default;
    ~LightBaker() = default;
    
    // ========================================================================
    // Main Baking Interface
    // ========================================================================
    
    // Bake lighting into BSP tree's lightmap atlas
    void Bake(BSPTree& bsp, const std::vector<BakeLight>& lights,
              const Options& options);
    
    // Convenience overload with default options
    void Bake(BSPTree& bsp, const std::vector<BakeLight>& lights) {
        Bake(bsp, lights, Options{});
    }
    
    // Bake using BSPTree's internal static lights (Phase 4D)
    void BakeWithSceneLights(BSPTree& bsp, const Options& options);
    void BakeWithSceneLights(BSPTree& bsp) {
        BakeWithSceneLights(bsp, Options{});
    }
    
    // Convert StaticLight to BakeLight
    static BakeLight ConvertStaticLight(const StaticLight& light);
    
    // Set progress callback
    void SetProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
    
    // ========================================================================
    // Utility
    // ========================================================================
    
    // Get last bake statistics
    struct BakeStats {
        uint32_t numFaces = 0;
        uint32_t numTexels = 0;
        uint32_t numShadowRays = 0;
        float bakeTimeSeconds = 0.0f;
        float atlasUtilization = 0.0f;
    };
    
    const BakeStats& GetLastStats() const { return m_stats; }
    
private:
    // Core baking
    void AllocateFaceLightmaps(BSPTree& bsp);
    void BakeFace(uint32_t faceIndex, BSPTree& bsp);
    
    // Lighting computation
    Vec3 ComputeTexelLighting(const Vec3& position, const Vec3& normal);
    float ComputeLightContribution(const BakeLight& light, const Vec3& pos, const Vec3& normal);
    
    // Shadow rays
    bool TraceShadowRay(const Vec3& from, const Vec3& to) const;
    
    // Face geometry helpers
    void ComputeFaceTangentSpace(const BSPFace& face, const BSPTree& bsp);
    Vec3 TexelToWorldPosition(const BSPFace& face, float u, float v) const;
    float ComputeFaceArea(const BSPFace& face, const BSPTree& bsp) const;
    
    // Process
    void PostProcessSmoothing(BSPTree& bsp);
    void DilateLightmaps(BSPTree& bsp);
    
private:
    Options m_options;
    std::vector<BakeLight> m_lights;
    BSPTree* m_bsp = nullptr;
    mutable BakeStats m_stats;  // mutable for shadow ray counting in const method
    ProgressCallback m_progressCallback;
    
    // Current face tangent space (computed per-face)
    Vec3 m_faceOrigin;
    Vec3 m_faceTangent;
    Vec3 m_faceBitangent;
    Vec3 m_faceNormal;
    Vec2 m_faceSize;  // World-space dimensions
    
    // Face lightmap allocation (atlas coordinates)
    struct FaceLightmapInfo {
        uint32_t atlasX, atlasY;
        uint32_t width, height;
        Vec2 uvMin, uvMax;
    };
    std::vector<FaceLightmapInfo> m_faceLightmaps;
};

// ============================================================================
// Lightmap File I/O
// ============================================================================

// Save lightmap atlas to binary file
bool SaveLightmapAtlas(const LightmapAtlas& atlas, const std::string& path);

// Load lightmap atlas from binary file
bool LoadLightmapAtlas(LightmapAtlas& atlas, const std::string& path);

} // namespace Genesis
