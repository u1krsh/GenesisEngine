#include "LightBaker.h"
#include "core/Logger.h"
#include "renderer/texture/Texture2D.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <random>

namespace Genesis {

// ============================================================================
// Main Baking Entry Point
// ============================================================================

void LightBaker::Bake(BSPTree& bsp, const std::vector<BakeLight>& lights,
                       const Options& options) {
    m_bsp = &bsp;
    m_lights = lights;
    m_options = options;
    m_stats = BakeStats();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (m_options.verbose) {
        LOG_INFO("LightBaker", "Starting light bake...");
        LOG_INFO("LightBaker", "  Faces: " + std::to_string(bsp.GetFaces().size()));
        LOG_INFO("LightBaker", "  Lights: " + std::to_string(lights.size()));
        LOG_INFO("LightBaker", "  Texels/Unit: " + std::to_string(options.texelsPerUnit));
    }
    
    // Step 1: Allocate lightmap regions for each face
    AllocateFaceLightmaps(bsp);
    
    // Step 2: Bake each face
    const auto& faces = bsp.GetFaces();
    m_stats.numFaces = static_cast<uint32_t>(faces.size());
    
    for (uint32_t i = 0; i < faces.size(); ++i) {
        BakeFace(i, bsp);
        
        if (m_progressCallback) {
            m_progressCallback(i + 1, static_cast<uint32_t>(faces.size()));
        }
    }
    
    // Step 3: Upload to GPU (Texture + Vertices)
    // Step 3: Post-process (Smoothing)
    PostProcessSmoothing(bsp);

    // Step 4: Dilation (Fill padding to prevent edge bleeding)
    DilateLightmaps(bsp);

    // Step 5: Upload to GPU
    bsp.UploadLightmapTexture();
    bsp.UpdateMesh(); // Critical: Sync vertex UVs to GPU!
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.bakeTimeSeconds = std::chrono::duration<float>(endTime - startTime).count();
    m_stats.atlasUtilization = bsp.GetLightmapAtlas().GetUtilization();
    
    if (m_options.verbose) {
        LOG_INFO("LightBaker", "Bake complete!");
        LOG_INFO("LightBaker", "  Time: " + std::to_string(m_stats.bakeTimeSeconds) + "s");
        LOG_INFO("LightBaker", "  Texels: " + std::to_string(m_stats.numTexels));
        LOG_INFO("LightBaker", "  Shadow rays: " + std::to_string(m_stats.numShadowRays));
        LOG_INFO("LightBaker", "  Atlas utilization: " + 
                 std::to_string(static_cast<int>(m_stats.atlasUtilization * 100)) + "%");
    }
}

// ============================================================================
// Static Light Conversion and Scene Light Baking (Phase 4D)
// ============================================================================

BakeLight LightBaker::ConvertStaticLight(const StaticLight& light) {
    BakeLight result;
    result.position = light.position;
    result.direction = light.direction;
    result.color = light.color;
    result.intensity = light.intensity;
    result.radius = light.radius;
    
    switch (light.type) {
        case StaticLightType::Point:
            result.type = BakeLightType::Point;
            break;
        case StaticLightType::Directional:
            result.type = BakeLightType::Directional;
            break;
    }
    
    return result;
}

void LightBaker::BakeWithSceneLights(BSPTree& bsp, const Options& options) {
    // Convert all StaticLights from BSPTree to BakeLights
    std::vector<BakeLight> bakeLights;
    bakeLights.reserve(bsp.GetLights().size());
    
    for (const auto& staticLight : bsp.GetLights()) {
        bakeLights.push_back(ConvertStaticLight(staticLight));
    }
    
    if (options.verbose) {
        LOG_INFO("LightBaker", "Converted " + std::to_string(bakeLights.size()) + 
                 " scene lights for baking");
    }
    
    // Bake with converted lights
    Bake(bsp, bakeLights, options);
}

// ============================================================================
// Lightmap Allocation
// ============================================================================

void LightBaker::AllocateFaceLightmaps(BSPTree& bsp) {
    LightmapAtlas& atlas = bsp.GetLightmapAtlas();
    atlas.Clear();
    
    auto& faces = bsp.GetFaces();  // Non-const for modification
    m_faceLightmaps.resize(faces.size());
    
    const int PADDING = 2; // Padding on each side
    
    for (uint32_t i = 0; i < faces.size(); ++i) {
        BSPFace& face = const_cast<BSPFace&>(faces[i]);
        
        // Compute lightmap size based on face area
        float area = ComputeFaceArea(face, bsp);
        float sideLength = std::sqrt(area);
        uint32_t lmSize = static_cast<uint32_t>(sideLength * m_options.texelsPerUnit);
        lmSize = std::clamp(lmSize, m_options.minLightmapSize, m_options.maxLightmapSize);
        
        // Allocation size includes padding
        uint32_t allocSize = lmSize + PADDING * 2;
        
        // Allocate in atlas
        FaceLightmapInfo& info = m_faceLightmaps[i];
        info.width = lmSize;
        info.height = lmSize;
        
        uint32_t outX, outY;
        if (atlas.AllocatePixels(allocSize, allocSize, outX, outY)) {
            // Store inner coordinates (where we bake to)
            info.atlasX = outX + PADDING;
            info.atlasY = outY + PADDING;
            
            // Convert to UVs for vertex assignment (Center on pixels)
            float invW = 1.0f / static_cast<float>(atlas.GetWidth());
            float invH = 1.0f / static_cast<float>(atlas.GetHeight());
            
            // Half-texel inset for safety
            info.uvMin = Vec2((info.atlasX + 0.5f) * invW, (info.atlasY + 0.5f) * invH);
            info.uvMax = Vec2((info.atlasX + lmSize - 0.5f) * invW, (info.atlasY + lmSize - 0.5f) * invH);
            
            // Store in face
            face.lightmapMins = info.uvMin;
            face.lightmapSize = info.uvMax - info.uvMin;
        }
    }
    
    if (m_options.verbose) {
        LOG_INFO("LightBaker", "Allocated " + std::to_string(faces.size()) + 
                 " face lightmaps in atlas");
    }
}

// ============================================================================
// Per-Face Baking
// ============================================================================

void LightBaker::BakeFace(uint32_t faceIndex, BSPTree& bsp) {
    const BSPFace& face = bsp.GetFaces()[faceIndex];
    const FaceLightmapInfo& info = m_faceLightmaps[faceIndex];
    LightmapAtlas& atlas = bsp.GetLightmapAtlas();
    
    // Compute tangent space for this face
    ComputeFaceTangentSpace(face, bsp);
    
    // Bake each texel
    for (uint32_t y = 0; y < info.height; ++y) {
        for (uint32_t x = 0; x < info.width; ++x) {
            // UV in face space (0-1)
            float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(info.width);
            float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(info.height);
            
            // Convert to world position
            Vec3 worldPos = TexelToWorldPosition(face, u, v);
            
            // Compute lighting
            Vec3 color = ComputeTexelLighting(worldPos, m_faceNormal);
            
            // Write to atlas
            atlas.SetPixel(info.atlasX + x, info.atlasY + y, color);
            m_stats.numTexels++;
        }
    }
    
    // Update vertex lightmap UVs
    auto& vertices = const_cast<std::vector<BSPVertex>&>(bsp.GetVertices());
    for (uint32_t i = 0; i < face.numVertices; ++i) {
        uint32_t vIdx = face.firstVertex + i;
        if (vIdx < vertices.size()) {
            // Simple planar projection for lightmap UVs
            BSPVertex& v = vertices[vIdx];
            Vec3 localPos = v.position - m_faceOrigin;
            float u = glm::dot(localPos, m_faceTangent) / m_faceSize.x;
            float s = glm::dot(localPos, m_faceBitangent) / m_faceSize.y;
            u = glm::clamp(u, 0.0f, 1.0f);
            s = glm::clamp(s, 0.0f, 1.0f);
            
            // Map to atlas UV
            v.lightmapCoord = glm::mix(info.uvMin, info.uvMax, Vec2(u, s));
        }
    }
}

// ============================================================================
// Lighting Computation
// ============================================================================

Vec3 LightBaker::ComputeTexelLighting(const Vec3& position, const Vec3& normal) {
    Vec3 result = Vec3(m_options.ambientLight);
    
    for (const auto& light : m_lights) {
        float contribution = ComputeLightContribution(light, position, normal);
        if (contribution > 0.0f) {
            result += light.color * light.intensity * contribution;
        }
    }
    
    // Clamp to reasonable HDR range
    return glm::clamp(result, Vec3(0.0f), Vec3(2.0f));
}

float LightBaker::ComputeLightContribution(const BakeLight& light, 
                                            const Vec3& pos, const Vec3& normal) {
    Vec3 lightDir;
    float attenuation = 1.0f;
    Vec3 lightPos = light.position; 
    
    switch (light.type) {
        case BakeLightType::Point:
        {
            Vec3 toLight = light.position - pos;
            float dist = glm::length(toLight);
            if (dist > light.radius) return 0.0f;
            
            lightDir = toLight / dist;
            
            // Inverse square falloff with radius cutoff
            float normDist = dist / light.radius;
            attenuation = 1.0f - normDist;
            attenuation *= attenuation;
            break;
        }
        
        case BakeLightType::Directional:
        {
            lightDir = -light.direction;
            lightPos = pos + lightDir * light.radius; 
            attenuation = 1.0f;
            break;
        }
        
        case BakeLightType::Spot:
        {
            Vec3 toLight = light.position - pos;
            float dist = glm::length(toLight);
            if (dist > light.radius) return 0.0f;
            
            lightDir = toLight / dist;
            
            // Cone falloff
            float cosAngle = glm::dot(-lightDir, light.direction);
            float coneAngle = glm::cos(glm::radians(light.spotAngle));
            if (cosAngle < coneAngle) return 0.0f;
            
            float spotFalloff = (cosAngle - coneAngle) / (1.0f - coneAngle);
            spotFalloff = glm::pow(spotFalloff, 1.0f / light.spotSoftness);
            
            float normDist = dist / light.radius;
            attenuation = (1.0f - normDist * normDist) * spotFalloff;
            break;
        }
    }
    
    // N·L check
    float NdotL = glm::dot(normal, lightDir);
    if (NdotL <= 0.0f) return 0.0f;
    
    // Hard Shadow (Source Engine Style: Step 2)
    Vec3 rayStart = pos + normal * m_options.shadowBias;
    if (TraceShadowRay(rayStart, lightPos)) {
        return 0.0f; // Occluded
    }
    
    return NdotL * attenuation;
}

// ============================================================================
// Shadow Rays
// ============================================================================

bool LightBaker::TraceShadowRay(const Vec3& from, const Vec3& to) const {
    m_stats.numShadowRays++;
    
    // Use per-pixel alpha shadow testing for glass faces
    float shadowAlpha = TraceShadowRayAlpha(from, to);
    
    // Also check solid brushes (non-glass)
    TraceResult result = m_bsp->GetCollision().TracePointIgnoreNoShadow(from, to);
    if (result.fraction < 0.99f) {
        return true;  // Hit solid geometry
    }
    
    // Block light if accumulated alpha is high enough
    return shadowAlpha > 0.5f;
}

// ============================================================================
// Per-Pixel Alpha Shadow Testing
// ============================================================================

bool LightBaker::RayTriangleIntersect(const Vec3& rayOrigin, const Vec3& rayDir,
                                       const Vec3& v0, const Vec3& v1, const Vec3& v2,
                                       float& t, float& u, float& v) const {
    // Möller–Trumbore intersection algorithm
    const float EPSILON = 0.0000001f;
    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    Vec3 h = glm::cross(rayDir, edge2);
    float a = glm::dot(edge1, h);
    
    if (a > -EPSILON && a < EPSILON) {
        return false;  // Ray is parallel to triangle
    }
    
    float f = 1.0f / a;
    Vec3 s = rayOrigin - v0;
    u = f * glm::dot(s, h);
    
    if (u < 0.0f || u > 1.0f) {
        return false;
    }
    
    Vec3 q = glm::cross(s, edge1);
    v = f * glm::dot(rayDir, q);
    
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }
    
    t = f * glm::dot(edge2, q);
    
    return t > EPSILON;
}

float LightBaker::TraceShadowRayAlpha(const Vec3& from, const Vec3& to) const {
    float accumulatedAlpha = 0.0f;
    Vec3 rayDir = to - from;
    float rayLength = glm::length(rayDir);
    if (rayLength < 0.001f) return 0.0f;
    rayDir /= rayLength;
    
    const auto& faces = m_bsp->GetFaces();
    const auto& vertices = m_bsp->GetVertices();
    const auto& materials = m_bsp->GetMaterials();
    const auto& indices = m_bsp->GetIndices();
    
    // Test against all glass faces
    for (uint32_t fi = 0; fi < faces.size(); ++fi) {
        const BSPFace& face = faces[fi];
        
        // Only check glass materials
        if (face.materialIndex >= materials.size()) continue;
        const std::string& matName = materials[face.materialIndex].name;
        if (matName.size() <= 7 || matName.substr(matName.size() - 7) != "__glass") {
            continue;
        }
        
        // Get material for texture sampling
        MaterialPtr mat = MaterialLibrary::Instance().Get(matName);
        if (!mat) continue;
        
        const TextureSlot* texSlot = mat->GetTextureSlot("u_BaseTexture");
        if (!texSlot || !texSlot->texture) continue;
        
        // Test all triangles in this face
        for (uint32_t i = 0; i + 2 < face.numIndices; i += 3) {
            uint32_t i0 = indices[face.firstIndex + i];
            uint32_t i1 = indices[face.firstIndex + i + 1];
            uint32_t i2 = indices[face.firstIndex + i + 2];
            
            if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
                continue;
            }
            
            const Vec3& v0 = vertices[i0].position;
            const Vec3& v1 = vertices[i1].position;
            const Vec3& v2 = vertices[i2].position;
            
            float t, u, v;
            if (RayTriangleIntersect(from, rayDir, v0, v1, v2, t, u, v)) {
                // Check if hit is within ray segment
                if (t > 0.0f && t < rayLength) {
                    // Interpolate texture coordinates
                    const Vec2& uv0 = vertices[i0].texCoord;
                    const Vec2& uv1 = vertices[i1].texCoord;
                    const Vec2& uv2 = vertices[i2].texCoord;
                    
                    float w = 1.0f - u - v;
                    Vec2 hitUV = uv0 * w + uv1 * u + uv2 * v;
                    
                    // Sample texture alpha
                    float alpha = texSlot->texture->SampleAlpha(hitUV.x, hitUV.y);
                    
                    // Accumulate shadow (opaque parts block more light)
                    accumulatedAlpha += alpha;
                }
            }
        }
    }
    
    return glm::clamp(accumulatedAlpha, 0.0f, 1.0f);
}

// ============================================================================
// Face Geometry Helpers
// ============================================================================

void LightBaker::ComputeFaceTangentSpace(const BSPFace& face, const BSPTree& bsp) {
    const auto& vertices = bsp.GetVertices();
    
    if (face.numVertices < 3) return;
    
    // Get first 3 vertices to compute tangent space
    const Vec3& v0 = vertices[face.firstVertex].position;
    const Vec3& v1 = vertices[face.firstVertex + 1].position;
    const Vec3& v2 = vertices[face.firstVertex + 2].position;
    
    m_faceNormal = face.plane.normal;
    m_faceOrigin = v0;
    
    // Compute tangent from edge
    m_faceTangent = glm::normalize(v1 - v0);
    m_faceBitangent = glm::normalize(glm::cross(m_faceNormal, m_faceTangent));
    
    // Compute face size by projecting all vertices
    float minU = 0, maxU = 0, minV = 0, maxV = 0;
    for (uint32_t i = 0; i < face.numVertices; ++i) {
        Vec3 local = vertices[face.firstVertex + i].position - m_faceOrigin;
        float u = glm::dot(local, m_faceTangent);
        float v = glm::dot(local, m_faceBitangent);
        minU = std::min(minU, u);
        maxU = std::max(maxU, u);
        minV = std::min(minV, v);
        maxV = std::max(maxV, v);
    }
    
    m_faceSize = Vec2(maxU - minU, maxV - minV);
    m_faceOrigin = m_faceOrigin + m_faceTangent * minU + m_faceBitangent * minV;
    
    // Prevent zero size
    if (m_faceSize.x < 0.001f) m_faceSize.x = 0.001f;
    if (m_faceSize.y < 0.001f) m_faceSize.y = 0.001f;
}

Vec3 LightBaker::TexelToWorldPosition(const BSPFace& face, float u, float v) const {
    return m_faceOrigin + 
           m_faceTangent * (u * m_faceSize.x) + 
           m_faceBitangent * (v * m_faceSize.y);
}

float LightBaker::ComputeFaceArea(const BSPFace& face, const BSPTree& bsp) const {
    const auto& vertices = bsp.GetVertices();
    
    if (face.numVertices < 3) return 1.0f;
    
    // Triangulate and sum areas
    float totalArea = 0.0f;
    const Vec3& v0 = vertices[face.firstVertex].position;
    
    for (uint32_t i = 1; i < face.numVertices - 1; ++i) {
        const Vec3& v1 = vertices[face.firstVertex + i].position;
        const Vec3& v2 = vertices[face.firstVertex + i + 1].position;
        
        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        totalArea += 0.5f * glm::length(glm::cross(edge1, edge2));
    }
    
    return totalArea;
}

// ============================================================================
// Lightmap File I/O
// ============================================================================

bool SaveLightmapAtlas(const LightmapAtlas& atlas, const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        LOG_ERROR("LightBaker", "Failed to open file for writing: " + path);
        return false;
    }
    
    // Header: magic, version, width, height
    const char magic[4] = {'L', 'M', 'A', 'P'};
    uint32_t version = 1;
    uint32_t width = atlas.GetWidth();
    uint32_t height = atlas.GetHeight();
    
    file.write(magic, 4);
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(&width), sizeof(width));
    file.write(reinterpret_cast<const char*>(&height), sizeof(height));
    
    // Pixel data (RGB8)
    file.write(reinterpret_cast<const char*>(atlas.GetPixelData()), atlas.GetPixelDataSize());
    
    LOG_INFO("LightBaker", "Saved lightmap atlas: " + path + 
             " (" + std::to_string(atlas.GetPixelDataSize() / 1024) + " KB)");
    
    return true;
}

bool LoadLightmapAtlas(LightmapAtlas& atlas, const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        LOG_ERROR("LightBaker", "Failed to open file for reading: " + path);
        return false;
    }
    
    // Read header
    char magic[4];
    uint32_t version, width, height;
    
    file.read(magic, 4);
    if (magic[0] != 'L' || magic[1] != 'M' || magic[2] != 'A' || magic[3] != 'P') {
        LOG_ERROR("LightBaker", "Invalid lightmap file format");
        return false;
    }
    
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&width), sizeof(width));
    file.read(reinterpret_cast<char*>(&height), sizeof(height));
    
    // Recreate atlas with correct size
    atlas = LightmapAtlas(width, height);
    
    // Read pixel data directly into atlas
    // Note: We need non-const access to write pixels
    std::vector<uint8_t> pixels(width * height * 3);
    file.read(reinterpret_cast<char*>(pixels.data()), pixels.size());
    
    // Copy pixels to atlas
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            size_t idx = (y * width + x) * 3;
            atlas.SetPixel(x, y, pixels[idx], pixels[idx + 1], pixels[idx + 2]);
        }
    }
    
    LOG_INFO("LightBaker", "Loaded lightmap atlas: " + path + 
             " (" + std::to_string(width) + "x" + std::to_string(height) + ")");
    
    return true;
}


// ============================================================================
// Post-Processing
// ============================================================================

void LightBaker::PostProcessSmoothing(BSPTree& bsp) {
    if (m_options.verbose) LOG_INFO("LightBaker", "Smoothing lightmaps...");

    LightmapAtlas& atlas = bsp.GetLightmapAtlas();
    
    // Create a copy of the source data for sampling
    const uint8_t* rawPixels = atlas.GetPixelData();
    if (!rawPixels) return;
    
    std::vector<uint8_t> srcPixels(rawPixels, rawPixels + atlas.GetPixelDataSize());
    
    int atlasWidth = atlas.GetWidth();
    // int atlasHeight = atlas.GetHeight();
    
    // For each face, blur its region
    for (const auto& info : m_faceLightmaps) {
        // Skip tiny faces
        if (info.width < 2 || info.height < 2) continue;
        
        for (uint32_t y = 0; y < info.height; ++y) {
            for (uint32_t x = 0; x < info.width; ++x) {
                Vec3 sumColor = Vec3(0.0f);
                float weightSum = 0.0f;
                
                // 3x3 Kernel
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        // Coordinates within face
                        int sx = static_cast<int>(x) + dx;
                        int sy = static_cast<int>(y) + dy;
                        
                        // CLAMP to face bounds
                        if (sx < 0) sx = 0;
                        if (sx >= static_cast<int>(info.width)) sx = info.width - 1;
                        if (sy < 0) sy = 0;
                        if (sy >= static_cast<int>(info.height)) sy = info.height - 1;
                        
                        // Atlas coordinates
                        int ax = info.atlasX + sx;
                        int ay = info.atlasY + sy;
                        
                        // Read from COPY
                        int idx = (ay * atlasWidth + ax) * 3;
                        if (idx >= 0 && idx + 2 < srcPixels.size()) {
                            Vec3 c(srcPixels[idx] / 255.0f, srcPixels[idx+1] / 255.0f, srcPixels[idx+2] / 255.0f);
                            sumColor += c;
                            weightSum += 1.0f;
                        }
                    }
                }
                
                if (weightSum > 0.0f) {
                    Vec3 blurred = sumColor / weightSum;
                    // Write to Atlas (which updates m_pixels)
                    // Note: SetPixel expects 0-255 or float, assumed float here based on usage
                    // Wait, SetPixel in Step 775 line 174 took Vec3 color.
                    // But loading code uses SetPixel(x,y, r,g,b).
                    // I will use SetPixel(x, y, Vec3) if available, otherwise check.
                    // BakeFace uses atlas.SetPixel(..., Vec3). So it's fine.
                    atlas.SetPixel(info.atlasX + x, info.atlasY + y, blurred);
                }
            }
        }
    }
}

void LightBaker::DilateLightmaps(BSPTree& bsp) {
    if (m_options.verbose) LOG_INFO("LightBaker", "Dilating lightmaps...");
    
    LightmapAtlas& atlas = bsp.GetLightmapAtlas();
    const int PADDING = 2;
    int atlasWidth = atlas.GetWidth();
    
    // Copy pixels for safe reading while writing
    const uint8_t* rawPixels = atlas.GetPixelData();
    const std::vector<uint8_t> srcPixels(rawPixels, rawPixels + atlas.GetPixelDataSize()); 
    
    auto CopyPixel = [&](int srcX, int srcY, int dstX, int dstY) {
        int srcIdx = (srcY * atlasWidth + srcX) * 3;
        if (srcIdx >= 0 && srcIdx + 2 < srcPixels.size()) {
             atlas.SetPixel(dstX, dstY, srcPixels[srcIdx], srcPixels[srcIdx+1], srcPixels[srcIdx+2]);
        }
    };
    
    for (const auto& info : m_faceLightmaps) {
        if (info.width == 0 || info.height == 0) continue;

        // Top edge (y=0) -> Dilate UP (y = -1..-P)
        for (uint32_t x = 0; x < info.width; ++x) {
            for (int p = 1; p <= PADDING; ++p) {
                CopyPixel(info.atlasX + x, info.atlasY, 
                          info.atlasX + x, info.atlasY - p);
            }
        }
        
        // Bottom edge (y=H-1) -> Dilate DOWN
        for (uint32_t x = 0; x < info.width; ++x) {
            for (int p = 1; p <= PADDING; ++p) {
                CopyPixel(info.atlasX + x, info.atlasY + info.height - 1, 
                          info.atlasX + x, info.atlasY + info.height - 1 + p);
            }
        }
        
        // Left edge (x=0) -> Dilate LEFT
        for (uint32_t y = 0; y < info.height; ++y) {
            for (int p = 1; p <= PADDING; ++p) {
                CopyPixel(info.atlasX, info.atlasY + y, 
                          info.atlasX - p, info.atlasY + y);
            }
        }

        // Right edge (x=W-1) -> Dilate RIGHT
        for (uint32_t y = 0; y < info.height; ++y) {
            for (int p = 1; p <= PADDING; ++p) {
                CopyPixel(info.atlasX + info.width - 1, info.atlasY + y, 
                          info.atlasX + info.width - 1 + p, info.atlasY + y);
            }
        }
        
        // Corners
        for (int p = 1; p <= PADDING; ++p) {
             CopyPixel(info.atlasX, info.atlasY, 
                       info.atlasX - p, info.atlasY - p); // TL
             CopyPixel(info.atlasX + info.width - 1, info.atlasY, 
                       info.atlasX + info.width - 1 + p, info.atlasY - p); // TR
             CopyPixel(info.atlasX, info.atlasY + info.height - 1, 
                       info.atlasX - p, info.atlasY + info.height - 1 + p); // BL
             CopyPixel(info.atlasX + info.width - 1, info.atlasY + info.height - 1, 
                       info.atlasX + info.width - 1 + p, info.atlasY + info.height - 1 + p); // BR
        }
    }
}
} // namespace Genesis
