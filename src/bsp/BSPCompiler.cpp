#include "BSPCompiler.h"
#include "BSPBuildVisualizer.h"
#include "core/Logger.h"
#include "renderer/material/Material.h"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include "math/Math.h"
#include <iostream>

namespace Genesis {

using Vec4 = glm::vec4;
using Mat4 = glm::mat4;

// ============================================================================
// CompileFace helpers
// ============================================================================

Vec3 BSPCompiler::CompileFace::GetCenter() const {
    if (vertices.empty()) return Vec3(0.0f);

    Vec3 center(0.0f);
    for (const auto& v : vertices) {
        center += v.position;
    }
    return center / static_cast<float>(vertices.size());
}

void BSPCompiler::CompileFace::CalculateBounds(Vec3& outMin, Vec3& outMax) const {
    if (vertices.empty()) {
        outMin = outMax = Vec3(0.0f);
        return;
    }

    outMin = outMax = vertices[0].position;
    for (size_t i = 1; i < vertices.size(); i++) {
        outMin = glm::min(outMin, vertices[i].position);
        outMax = glm::max(outMax, vertices[i].position);
    }
}

// ============================================================================
// BSPCompiler Implementation
// ============================================================================

BSPTreePtr BSPCompiler::Compile(const Map& map) {
    return Compile(map, Options());
}

BSPTreePtr BSPCompiler::Compile(const Map& map, const Options& options) {
    m_options = options;
    m_bsp = std::make_shared<BSPTree>();
    m_materialMap.clear();
    m_planeHashMap.clear();
    m_currentDepth = 0;
    m_maxDepthReached = 0;
    m_planesReused = 0;
    m_lastError.clear();

    if (m_options.verbose) {
        std::cout << "[BSPCompiler] Starting compilation..." << std::endl;
        std::cout << "[BSPCompiler] Input: " << map.GetBrushCount() << " brushes" << std::endl;
    }

    // Step 1: Convert all brushes to compile faces
    std::vector<CompileFace> allFaces;

    uint32_t brushIndex = 0;
    for (const auto& brush : map.GetBrushes()) {
        if (!brush.IsVisible()) continue;

        BrushToFaces(brush, brushIndex, allFaces);
        brushIndex++;
    }

    if (m_options.verbose) {
        std::cout << "[BSPCompiler] Generated " << allFaces.size() << " faces" << std::endl;
    }

    if (allFaces.empty()) {
        m_lastError = "No visible brushes to compile";
        return nullptr;
    }

    // Start recording for visualization
    auto& visualizer = BSPBuildVisualizer::Instance();
    visualizer.StartRecording();

    // Calculate world bounds for visualization
    Vec3 worldMin(FLT_MAX), worldMax(-FLT_MAX);
    for (const auto& face : allFaces) {
        for (const auto& v : face.vertices) {
            worldMin = glm::min(worldMin, v.position);
            worldMax = glm::max(worldMax, v.position);
        }
    }
    visualizer.SetWorldBounds(worldMin, worldMax);

    // Record static geometry (all initial faces)
    for (const auto& face : allFaces) {
        if (face.vertices.size() < 2) continue;
        
        for (size_t i = 0; i < face.vertices.size(); i++) {
            size_t next = (i + 1) % face.vertices.size();
            // Only add wall-like edges (mostly vertical normal) or just all edges?
            // User wants "the map". All edges is safer.
            visualizer.AddStaticLine(face.vertices[i].position, face.vertices[next].position);
        }
    }

    // This is basically a flat list, but sets up the structure for future phases

    int32_t rootNode;
    if (m_options.balanceTree && allFaces.size() > m_options.maxLeafFaces) {
        // Build actual BSP tree
        rootNode = BuildTree(allFaces, 0);
    } else {
        // Simple case: all faces in one leaf
        uint32_t leafIdx = CreateLeaf(allFaces);
        rootNode = -(static_cast<int32_t>(leafIdx) + 1);  // Negative = leaf
    }

    // Store the root node index
    m_bsp->SetRootNode(rootNode);

    // Stop recording visualization
    visualizer.StopRecording();

    // Phase 2: Generate collision hulls
    GenerateCollisionHulls(map);

    // Set the BSP tree pointer in the collision system
    m_bsp->GetCollision().SetBSPTree(m_bsp.get());
    
    // Build spatial grid for O(log n) collision queries
    m_bsp->GetCollision().BuildGrid();

    // Phase 3: Build PVS (Potentially Visible Set)
    if (m_options.buildPVS) {
        if (m_options.verbose) {
            std::cout << "[BSPCompiler] Building PVS..." << std::endl;
        }
        m_bsp->GetPVS().Build(*m_bsp);
        if (m_options.verbose) {
            std::cout << "[BSPCompiler] PVS built! Avg visibility: " 
                      << static_cast<int>(m_bsp->GetPVS().GetAverageVisibility() * 100) << "%" << std::endl;
        }
    }

    if (m_options.verbose) {
        BSPStats stats = m_bsp->GetStats();
        std::cout << "[BSPCompiler] Compilation complete!" << std::endl;
        std::cout << "[BSPCompiler] Collision brushes: " << m_bsp->GetCollision().GetBrushCount() << std::endl;
        std::cout << "[BSPCompiler] Collision planes: " << m_bsp->GetCollision().GetPlaneCount() << std::endl;
        stats.Print();
    }

    return m_bsp;
}

// ============================================================================
// Brush to Faces conversion
// ============================================================================

void BSPCompiler::BrushToFaces(const Brush& brush, uint32_t brushIndex, std::vector<CompileFace>& outFaces) {
    switch (brush.shape) {
        case BrushShape::Cube:
            GenerateCubeFaces(brush, brushIndex, outFaces);
            break;
        case BrushShape::Sphere:
            GenerateSphereFaces(brush, brushIndex, outFaces);
            break;
        case BrushShape::Cylinder:
            GenerateCylinderFaces(brush, brushIndex, outFaces);
            break;
        case BrushShape::Cone:
            GenerateConeFaces(brush, brushIndex, outFaces);
            break;
        default:
            // Default to cube for unknown shapes
            GenerateCubeFaces(brush, brushIndex, outFaces);
            break;
    }
}

void BSPCompiler::GenerateCubeFaces(const Brush& brush, uint32_t brushIndex, std::vector<CompileFace>& outFaces) {
    Vec3 pos = brush.position;
    Vec3 halfSize = brush.size * 0.5f;
    // Use same material key format as MapLoader: materialName__shaderType
    std::string materialKey = brush.materialName;
    if (brush.shaderType == ShaderType::Glass) {
        materialKey += "__glass";
    }
    uint32_t matIdx = GetMaterialIndex(materialKey);

    // Apply rotation if needed
    Mat4 rotMat = Mat4(1.0f);
    if (brush.rotation != Vec3(0.0f)) {
        rotMat = glm::rotate(rotMat, glm::radians(brush.rotation.x), Vec3(1, 0, 0));
        rotMat = glm::rotate(rotMat, glm::radians(brush.rotation.y), Vec3(0, 1, 0));
        rotMat = glm::rotate(rotMat, glm::radians(brush.rotation.z), Vec3(0, 0, 1));
    }

    auto transformPoint = [&](const Vec3& local) -> Vec3 {
        Vec4 p = rotMat * Vec4(local, 1.0f);
        return pos + Vec3(p);
    };

    auto transformNormal = [&](const Vec3& n) -> Vec3 {
        Vec4 tn = rotMat * Vec4(n, 0.0f);
        return glm::normalize(Vec3(tn));
    };

    // 8 corners of the cube (local space)
    Vec3 corners[8] = {
        Vec3(-halfSize.x, -halfSize.y, -halfSize.z), // 0: left-bottom-back
        Vec3( halfSize.x, -halfSize.y, -halfSize.z), // 1: right-bottom-back
        Vec3( halfSize.x,  halfSize.y, -halfSize.z), // 2: right-top-back
        Vec3(-halfSize.x,  halfSize.y, -halfSize.z), // 3: left-top-back
        Vec3(-halfSize.x, -halfSize.y,  halfSize.z), // 4: left-bottom-front
        Vec3( halfSize.x, -halfSize.y,  halfSize.z), // 5: right-bottom-front
        Vec3( halfSize.x,  halfSize.y,  halfSize.z), // 6: right-top-front
        Vec3(-halfSize.x,  halfSize.y,  halfSize.z), // 7: left-top-front
    };

    // Transform corners to world space
    for (int i = 0; i < 8; i++) {
        corners[i] = transformPoint(corners[i]);
    }

    // 6 faces of the cube
    struct FaceDef {
        int indices[4];
        Vec3 normal;
    };

    FaceDef faces[6] = {
        // Front face (+Z)
        {{4, 5, 6, 7}, Vec3(0, 0, 1)},
        // Back face (-Z)
        {{1, 0, 3, 2}, Vec3(0, 0, -1)},
        // Right face (+X)
        {{5, 1, 2, 6}, Vec3(1, 0, 0)},
        // Left face (-X)
        {{0, 4, 7, 3}, Vec3(-1, 0, 0)},
        // Top face (+Y)
        {{7, 6, 2, 3}, Vec3(0, 1, 0)},
        // Bottom face (-Y)
        {{0, 1, 5, 4}, Vec3(0, -1, 0)},
    };

    for (int f = 0; f < 6; f++) {
        CompileFace face;
        face.materialIndex = matIdx;
        face.brushIndex = brushIndex;

        Vec3 normal = transformNormal(faces[f].normal);
        face.plane = BSPPlane(normal, corners[faces[f].indices[0]]);

        // Add 4 vertices for quad
        for (int v = 0; v < 4; v++) {
            BSPVertex vert;
            vert.position = corners[faces[f].indices[v]];
            vert.normal = normal;

            // Simple UV mapping based on face orientation
            if (std::abs(normal.y) > 0.9f) {
                // Top/bottom face - use XZ
                vert.texCoord = Vec2(vert.position.x, vert.position.z);
            } else if (std::abs(normal.x) > 0.9f) {
                // Left/right face - use ZY
                vert.texCoord = Vec2(vert.position.z, vert.position.y);
            } else {
                // Front/back face - use XY
                vert.texCoord = Vec2(vert.position.x, vert.position.y);
            }

            face.vertices.push_back(vert);
        }

        outFaces.push_back(std::move(face));
    }
}

void BSPCompiler::GenerateSphereFaces(const Brush& brush, uint32_t brushIndex, std::vector<CompileFace>& outFaces) {
    Vec3 pos = brush.position;
    Vec3 scale = brush.size * 0.5f;
    // Use same material key format as MapLoader
    std::string materialKey = brush.materialName;
    if (brush.shaderType == ShaderType::Glass) {
        materialKey += "__glass";
    }
    uint32_t matIdx = GetMaterialIndex(materialKey);

    const int segments = 16;
    const int rings = 12;
    const float PI = 3.14159265358979323846f;

    // Generate sphere as triangle strips converted to quads
    for (int ring = 0; ring < rings; ring++) {
        float theta1 = PI * ring / rings;
        float theta2 = PI * (ring + 1) / rings;

        for (int seg = 0; seg < segments; seg++) {
            float phi1 = 2.0f * PI * seg / segments;
            float phi2 = 2.0f * PI * (seg + 1) / segments;

            // Four corners of the quad
            auto spherePoint = [&](float theta, float phi) -> Vec3 {
                return pos + Vec3(
                    scale.x * std::sin(theta) * std::cos(phi),
                    scale.y * std::cos(theta),
                    scale.z * std::sin(theta) * std::sin(phi)
                );
            };

            auto sphereNormal = [&](float theta, float phi) -> Vec3 {
                return glm::normalize(Vec3(
                    std::sin(theta) * std::cos(phi),
                    std::cos(theta),
                    std::sin(theta) * std::sin(phi)
                ));
            };

            Vec3 p0 = spherePoint(theta1, phi1);
            Vec3 p1 = spherePoint(theta1, phi2);
            Vec3 p2 = spherePoint(theta2, phi2);
            Vec3 p3 = spherePoint(theta2, phi1);

            Vec3 n0 = sphereNormal(theta1, phi1);
            Vec3 n1 = sphereNormal(theta1, phi2);
            Vec3 n2 = sphereNormal(theta2, phi2);
            Vec3 n3 = sphereNormal(theta2, phi1);

            // Skip degenerate quads at poles
            if (ring == 0) {
                // Top triangle
                CompileFace face;
                face.materialIndex = matIdx;
                face.brushIndex = brushIndex;

                Vec3 faceNormal = glm::normalize(n0 + n2 + n3);
                face.plane = BSPPlane(faceNormal, p0);

                face.vertices.push_back({p0, n0, Vec2(phi1 / (2*PI), theta1 / PI)});
                face.vertices.push_back({p2, n2, Vec2(phi2 / (2*PI), theta2 / PI)});
                face.vertices.push_back({p3, n3, Vec2(phi1 / (2*PI), theta2 / PI)});

                outFaces.push_back(std::move(face));
            } else if (ring == rings - 1) {
                // Bottom triangle
                CompileFace face;
                face.materialIndex = matIdx;
                face.brushIndex = brushIndex;

                Vec3 faceNormal = glm::normalize(n0 + n1 + n2);
                face.plane = BSPPlane(faceNormal, p0);

                face.vertices.push_back({p0, n0, Vec2(phi1 / (2*PI), theta1 / PI)});
                face.vertices.push_back({p1, n1, Vec2(phi2 / (2*PI), theta1 / PI)});
                face.vertices.push_back({p2, n2, Vec2(phi2 / (2*PI), theta2 / PI)});

                outFaces.push_back(std::move(face));
            } else {
                // Regular quad
                CompileFace face;
                face.materialIndex = matIdx;
                face.brushIndex = brushIndex;

                Vec3 faceNormal = glm::normalize(n0 + n1 + n2 + n3);
                face.plane = BSPPlane(faceNormal, p0);

                face.vertices.push_back({p0, n0, Vec2(phi1 / (2*PI), theta1 / PI)});
                face.vertices.push_back({p1, n1, Vec2(phi2 / (2*PI), theta1 / PI)});
                face.vertices.push_back({p2, n2, Vec2(phi2 / (2*PI), theta2 / PI)});
                face.vertices.push_back({p3, n3, Vec2(phi1 / (2*PI), theta2 / PI)});

                outFaces.push_back(std::move(face));
            }
        }
    }
}

void BSPCompiler::GenerateCylinderFaces(const Brush& brush, uint32_t brushIndex, std::vector<CompileFace>& outFaces) {
    Vec3 pos = brush.position;
    Vec3 scale = brush.size * 0.5f;
    // Use same material key format as MapLoader
    std::string materialKey = brush.materialName;
    if (brush.shaderType == ShaderType::Glass) {
        materialKey += "__glass";
    }
    uint32_t matIdx = GetMaterialIndex(materialKey);

    const int segments = 16;
    const float PI = 3.14159265358979323846f;

    // Side faces
    for (int seg = 0; seg < segments; seg++) {
        float phi1 = 2.0f * PI * seg / segments;
        float phi2 = 2.0f * PI * (seg + 1) / segments;

        float c1 = std::cos(phi1), s1 = std::sin(phi1);
        float c2 = std::cos(phi2), s2 = std::sin(phi2);

        Vec3 p0 = pos + Vec3(scale.x * c1, -scale.y, scale.z * s1);
        Vec3 p1 = pos + Vec3(scale.x * c2, -scale.y, scale.z * s2);
        Vec3 p2 = pos + Vec3(scale.x * c2,  scale.y, scale.z * s2);
        Vec3 p3 = pos + Vec3(scale.x * c1,  scale.y, scale.z * s1);

        Vec3 n0 = glm::normalize(Vec3(c1, 0, s1));
        Vec3 n1 = glm::normalize(Vec3(c2, 0, s2));

        CompileFace face;
        face.materialIndex = matIdx;
        face.brushIndex = brushIndex;
        face.plane = BSPPlane(glm::normalize(n0 + n1), (p0 + p2) * 0.5f);

        face.vertices.push_back({p0, n0, Vec2(phi1 / (2*PI), 0)});
        face.vertices.push_back({p1, n1, Vec2(phi2 / (2*PI), 0)});
        face.vertices.push_back({p2, n1, Vec2(phi2 / (2*PI), 1)});
        face.vertices.push_back({p3, n0, Vec2(phi1 / (2*PI), 1)});

        outFaces.push_back(std::move(face));
    }

    // Top cap
    CompileFace topFace;
    topFace.materialIndex = matIdx;
    topFace.brushIndex = brushIndex;
    topFace.plane = BSPPlane(Vec3(0, 1, 0), pos + Vec3(0, scale.y, 0));

    for (int seg = 0; seg < segments; seg++) {
        float phi = 2.0f * PI * seg / segments;
        Vec3 p = pos + Vec3(scale.x * std::cos(phi), scale.y, scale.z * std::sin(phi));
        topFace.vertices.push_back({p, Vec3(0, 1, 0), Vec2(std::cos(phi) * 0.5f + 0.5f, std::sin(phi) * 0.5f + 0.5f)});
    }
    outFaces.push_back(std::move(topFace));

    // Bottom cap (reversed winding)
    CompileFace bottomFace;
    bottomFace.materialIndex = matIdx;
    bottomFace.brushIndex = brushIndex;
    bottomFace.plane = BSPPlane(Vec3(0, -1, 0), pos + Vec3(0, -scale.y, 0));

    for (int seg = segments - 1; seg >= 0; seg--) {
        float phi = 2.0f * PI * seg / segments;
        Vec3 p = pos + Vec3(scale.x * std::cos(phi), -scale.y, scale.z * std::sin(phi));
        bottomFace.vertices.push_back({p, Vec3(0, -1, 0), Vec2(std::cos(phi) * 0.5f + 0.5f, std::sin(phi) * 0.5f + 0.5f)});
    }
    outFaces.push_back(std::move(bottomFace));
}

void BSPCompiler::GenerateConeFaces(const Brush& brush, uint32_t brushIndex, std::vector<CompileFace>& outFaces) {
    Vec3 pos = brush.position;
    Vec3 scale = brush.size * 0.5f;
    // Use same material key format as MapLoader
    std::string materialKey = brush.materialName;
    if (brush.shaderType == ShaderType::Glass) {
        materialKey += "__glass";
    }
    uint32_t matIdx = GetMaterialIndex(materialKey);

    const int segments = 16;
    const float PI = 3.14159265358979323846f;

    Vec3 apex = pos + Vec3(0, scale.y, 0);

    // Side triangles
    for (int seg = 0; seg < segments; seg++) {
        float phi1 = 2.0f * PI * seg / segments;
        float phi2 = 2.0f * PI * (seg + 1) / segments;

        float c1 = std::cos(phi1), s1 = std::sin(phi1);
        float c2 = std::cos(phi2), s2 = std::sin(phi2);

        Vec3 p1 = pos + Vec3(scale.x * c1, -scale.y, scale.z * s1);
        Vec3 p2 = pos + Vec3(scale.x * c2, -scale.y, scale.z * s2);

        // Calculate proper cone normal
        Vec3 edge1 = apex - p1;
        Vec3 edge2 = p2 - p1;
        Vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

        CompileFace face;
        face.materialIndex = matIdx;
        face.brushIndex = brushIndex;
        face.plane = BSPPlane(faceNormal, p1);

        // Vertex normals for smooth shading
        float slopeAngle = std::atan2(scale.y, scale.x);
        Vec3 n1 = glm::normalize(Vec3(c1 * std::cos(slopeAngle), std::sin(slopeAngle), s1 * std::cos(slopeAngle)));
        Vec3 n2 = glm::normalize(Vec3(c2 * std::cos(slopeAngle), std::sin(slopeAngle), s2 * std::cos(slopeAngle)));

        face.vertices.push_back({apex, glm::normalize(n1 + n2), Vec2(0.5f, 1)});
        face.vertices.push_back({p1, n1, Vec2(phi1 / (2*PI), 0)});
        face.vertices.push_back({p2, n2, Vec2(phi2 / (2*PI), 0)});

        outFaces.push_back(std::move(face));
    }

    // Bottom cap
    CompileFace bottomFace;
    bottomFace.materialIndex = matIdx;
    bottomFace.brushIndex = brushIndex;
    bottomFace.plane = BSPPlane(Vec3(0, -1, 0), pos + Vec3(0, -scale.y, 0));

    for (int seg = segments - 1; seg >= 0; seg--) {
        float phi = 2.0f * PI * seg / segments;
        Vec3 p = pos + Vec3(scale.x * std::cos(phi), -scale.y, scale.z * std::sin(phi));
        bottomFace.vertices.push_back({p, Vec3(0, -1, 0), Vec2(std::cos(phi) * 0.5f + 0.5f, std::sin(phi) * 0.5f + 0.5f)});
    }
    outFaces.push_back(std::move(bottomFace));
}

// ============================================================================
// BSP Tree Building
// ============================================================================

int32_t BSPCompiler::BuildTree(std::vector<CompileFace>& faces, uint32_t depth) {
    m_currentDepth = depth;
    if (depth > m_maxDepthReached) {
        m_maxDepthReached = depth;
    }

    // Stop conditions
    if (faces.empty()) {
        return -1;
    }

    if (depth >= m_options.maxTreeDepth || faces.size() <= m_options.maxLeafFaces) {
        // Create leaf
        uint32_t leafIdx = CreateLeaf(faces);
        return -(static_cast<int32_t>(leafIdx) + 1);  // Negative = leaf
    }

    // Choose split plane
    uint32_t splitFaceIdx = ChooseSplitPlane(faces);
    if (splitFaceIdx >= faces.size()) {
        // Couldn't find good split, make leaf
        uint32_t leafIdx = CreateLeaf(faces);
        return -(static_cast<int32_t>(leafIdx) + 1);
    }

    BSPPlane splitPlane = faces[splitFaceIdx].plane;
    uint32_t planeIdx = GetPlaneIndex(splitPlane);

    // Record step for visualization
    if (BSPBuildVisualizer::Instance().IsRecording()) {
        BSPBuildStep step;
        step.type = BSPBuildStep::Type::SplitPlane;
        
        // Calculate bounds of current region
        step.boundsMin = Vec3(FLT_MAX);
        step.boundsMax = Vec3(-FLT_MAX);
        for (const auto& f : faces) {
            for (const auto& v : f.vertices) {
                step.boundsMin = glm::min(step.boundsMin, v.position);
                step.boundsMax = glm::max(step.boundsMax, v.position);
            }
        }
        
        step.planeNormal = splitPlane.normal;
        step.planePoint = splitPlane.normal * splitPlane.distance; // Approximate point on plane
        
        BSPBuildVisualizer::Instance().AddStep(step);
    }

    // Partition faces
    std::vector<CompileFace> frontFaces, backFaces;

    for (auto& face : faces) {
        FaceClassification classify = ClassifyFace(face, splitPlane);

        switch (classify) {
            case FaceClassification::Front:
            case FaceClassification::OnPlane:
                frontFaces.push_back(std::move(face));
                break;
            case FaceClassification::Back:
                backFaces.push_back(std::move(face));
                break;
            case FaceClassification::Spanning:
                {
                    CompileFace frontFace, backFace;
                    SplitFace(face, splitPlane, frontFace, backFace);
                    if (!frontFace.vertices.empty()) frontFaces.push_back(std::move(frontFace));
                    if (!backFace.vertices.empty()) backFaces.push_back(std::move(backFace));
                }
                break;
        }
    }

    // Check for degenerate cases
    if (frontFaces.empty() || backFaces.empty()) {
        // Bad split, just make a leaf
        faces = frontFaces.empty() ? std::move(backFaces) : std::move(frontFaces);
        uint32_t leafIdx = CreateLeaf(faces);
        return -(static_cast<int32_t>(leafIdx) + 1);
    }

    // Create node
    BSPNode node;
    node.planeIndex = planeIdx;

    // Build children
    node.frontChild = BuildTree(frontFaces, depth + 1);
    node.backChild = BuildTree(backFaces, depth + 1);

    // Calculate node bounds by merging children bounds
    node.boundsMin = Vec3(std::numeric_limits<float>::max());
    node.boundsMax = Vec3(std::numeric_limits<float>::lowest());

    auto ExpandBounds = [&](int32_t childIndex) {
        Vec3 cMin, cMax;
        if (childIndex < 0) {
            // Leaf
            uint32_t idx = static_cast<uint32_t>(-(childIndex + 1));
            const auto& leaf = m_bsp->GetLeafs()[idx];
            cMin = leaf.boundsMin;
            cMax = leaf.boundsMax;
        } else {
            // Node
            const auto& n = m_bsp->GetNodes()[childIndex];
            cMin = n.boundsMin;
            cMax = n.boundsMax;
        }
        node.boundsMin = glm::min(node.boundsMin, cMin);
        node.boundsMax = glm::max(node.boundsMax, cMax);
    };

    ExpandBounds(node.frontChild);
    ExpandBounds(node.backChild);

    uint32_t nodeIdx = static_cast<uint32_t>(m_bsp->GetNodes().size());
    m_bsp->GetNodes().push_back(node);

    return static_cast<int32_t>(nodeIdx);
}

void BSPCompiler::SplitFace(const CompileFace& input, const BSPPlane& plane, CompileFace& outFront, CompileFace& outBack) {
    outFront.materialIndex = input.materialIndex;
    outFront.brushIndex = input.brushIndex;
    outFront.plane = input.plane;
    
    outBack.materialIndex = input.materialIndex;
    outBack.brushIndex = input.brushIndex;
    outBack.plane = input.plane;
    
    const auto& verts = input.vertices;
    for (size_t i = 0; i < verts.size(); ++i) {
        size_t nextIdx = (i + 1) % verts.size();
        const auto& v1 = verts[i];
        const auto& v2 = verts[nextIdx];
        
        float d1 = glm::dot(plane.normal, v1.position) - plane.distance;
        float d2 = glm::dot(plane.normal, v2.position) - plane.distance;
        
        bool v1Front = d1 > -m_options.splitEpsilon;
        bool v1Back = d1 < m_options.splitEpsilon;
        
        bool v2Front = d2 > -m_options.splitEpsilon;
        bool v2Back = d2 < m_options.splitEpsilon;
        
        if (v1Front) outFront.vertices.push_back(v1);
        if (v1Back) outBack.vertices.push_back(v1);
        
        // Check for crossing
        // If one is strictly front and other strictly back (or vice versa)
        if ((d1 > m_options.splitEpsilon && d2 < -m_options.splitEpsilon) || 
            (d1 < -m_options.splitEpsilon && d2 > m_options.splitEpsilon)) {
            
            // Calculate intersection
            float t = d1 / (d1 - d2);
            Vec3 p = v1.position + (v2.position - v1.position) * t;
            Vec3 n = glm::normalize(v1.normal + (v2.normal - v1.normal) * t); // Simple linear interpolate for normal
            Vec2 uv = v1.texCoord + (v2.texCoord - v1.texCoord) * t;
            
            BSPVertex intersectionVert = { p, n, uv };
            outFront.vertices.push_back(intersectionVert);
            outBack.vertices.push_back(intersectionVert);
        }
    }
}


uint32_t BSPCompiler::ChooseSplitPlane(const std::vector<CompileFace>& faces) {
    if (faces.empty()) return UINT32_MAX;

    // SAH (Surface Area Heuristic) based split plane selection
    // Penalizes polygon splits and prefers balanced trees
    
    float bestCost = std::numeric_limits<float>::max();
    uint32_t bestIdx = 0;
    
    // Sample a subset of faces for large sets (performance optimization)
    uint32_t sampleStep = 1;
    if (faces.size() > 64) {
        sampleStep = static_cast<uint32_t>(faces.size()) / 32;  // Sample ~32 planes
    }
    
    for (uint32_t i = 0; i < faces.size(); i += sampleStep) {
        uint32_t frontCount = 0, backCount = 0, splitCount = 0;
        float cost = EvaluateSplitCost(faces, faces[i].plane, frontCount, backCount, splitCount);
        
        // Skip degenerate splits
        if (frontCount == 0 || backCount == 0) {
            cost = std::numeric_limits<float>::max();  // Penalize one-sided splits
        }
        
        if (cost < bestCost) {
            bestCost = cost;
            bestIdx = i;
        }
    }

    return bestIdx;
}

float BSPCompiler::EvaluateSplitCost(const std::vector<CompileFace>& faces,
                                      const BSPPlane& plane,
                                      uint32_t& outFrontCount,
                                      uint32_t& outBackCount,
                                      uint32_t& outSplitCount) {
    outFrontCount = 0;
    outBackCount = 0;
    outSplitCount = 0;
    
    for (const auto& face : faces) {
        FaceClassification c = ClassifyFace(face, plane);
        switch (c) {
            case FaceClassification::Front:
            case FaceClassification::OnPlane:
                outFrontCount++;
                break;
            case FaceClassification::Back:
                outBackCount++;
                break;
            case FaceClassification::Spanning:
                outSplitCount++;
                outFrontCount++;  // Split produces front piece
                outBackCount++;   // Split produces back piece
                break;
        }
    }
    
    // SAH cost formula:
    // - Splits are expensive (each adds 2 new faces)
    // - Balanced trees are preferred (minimize max child size)
    // - Slight preference for axis-aligned planes (implicit in face planes)
    
    constexpr float SPLIT_PENALTY = 8.0f;    // High cost for splitting polygons
    constexpr float BALANCE_WEIGHT = 1.0f;   // Weight for tree balance
    
    // Imbalance cost: prefer 50/50 splits
    float totalFaces = static_cast<float>(outFrontCount + outBackCount);
    float imbalance = 0.0f;
    if (totalFaces > 0.0f) {
        float ratio = std::abs(static_cast<float>(outFrontCount) - static_cast<float>(outBackCount)) / totalFaces;
        imbalance = ratio * BALANCE_WEIGHT * totalFaces;
    }
    
    // Split cost: each split creates 2 new polygons
    float splitCost = static_cast<float>(outSplitCount) * SPLIT_PENALTY;
    
    return imbalance + splitCost;
}

BSPCompiler::FaceClassification BSPCompiler::ClassifyFace(const CompileFace& face, const BSPPlane& plane) {
    int front = 0, back = 0, on = 0;

    for (const auto& vert : face.vertices) {
        int side = plane.ClassifyPointEpsilon(vert.position, m_options.splitEpsilon);
        if (side > 0) front++;
        else if (side < 0) back++;
        else on++;
    }

    if (front > 0 && back == 0) return FaceClassification::Front;
    if (back > 0 && front == 0) return FaceClassification::Back;
    if (front == 0 && back == 0) return FaceClassification::OnPlane;
    return FaceClassification::Spanning;
}

uint32_t BSPCompiler::CreateLeaf(std::vector<CompileFace>& faces) {
    BSPLeaf leaf;
    leaf.contents = BSPContents::Empty;

    if (faces.empty()) {
        leaf.firstFace = 0;
        leaf.numFaces = 0;
    } else {
        leaf.firstFace = static_cast<uint32_t>(m_bsp->GetLeafFaces().size());
        leaf.numFaces = static_cast<uint32_t>(faces.size());

        // Calculate bounds
        leaf.boundsMin = Vec3(std::numeric_limits<float>::max());
        leaf.boundsMax = Vec3(std::numeric_limits<float>::lowest());

        for (auto& compileFace : faces) {
            // Add face to BSP
            BSPFace bspFace;
            TriangulateFace(compileFace, bspFace);

            uint32_t faceIdx = static_cast<uint32_t>(m_bsp->GetFaces().size());
            m_bsp->GetFaces().push_back(bspFace);
            m_bsp->GetLeafFaces().push_back(faceIdx);

            // Update bounds
            for (const auto& vert : compileFace.vertices) {
                leaf.boundsMin = glm::min(leaf.boundsMin, vert.position);
                leaf.boundsMax = glm::max(leaf.boundsMax, vert.position);
            }
        }
    }

    uint32_t leafIdx = static_cast<uint32_t>(m_bsp->GetLeafs().size());
    m_bsp->GetLeafs().push_back(leaf);

    // Record step for visualization
    if (BSPBuildVisualizer::Instance().IsRecording()) {
        BSPBuildStep step;
        step.type = BSPBuildStep::Type::CreateLeaf;
        step.boundsMin = leaf.boundsMin;
        step.boundsMax = leaf.boundsMax;
        step.nodeIndex = leafIdx;
        BSPBuildVisualizer::Instance().AddStep(step);
    }

    return leafIdx;
}

void BSPCompiler::TriangulateFace(const CompileFace& face, BSPFace& outFace) {
    if (face.vertices.empty()) return;

    outFace.plane = face.plane;
    outFace.materialIndex = face.materialIndex;
    outFace.brushIndex = face.brushIndex;
    outFace.firstVertex = static_cast<uint32_t>(m_bsp->GetVertices().size());
    outFace.numVertices = static_cast<uint32_t>(face.vertices.size());
    outFace.firstIndex = static_cast<uint32_t>(m_bsp->GetIndices().size());

    // Add vertices
    for (const auto& vert : face.vertices) {
        m_bsp->GetVertices().push_back(vert);
    }

    // Triangulate as fan
    // For a polygon with N vertices, we create N-2 triangles
    if (face.vertices.size() >= 3) {
        for (size_t i = 1; i < face.vertices.size() - 1; i++) {
            m_bsp->GetIndices().push_back(outFace.firstVertex);
            m_bsp->GetIndices().push_back(outFace.firstVertex + static_cast<uint32_t>(i));
            m_bsp->GetIndices().push_back(outFace.firstVertex + static_cast<uint32_t>(i + 1));
        }
        outFace.numIndices = static_cast<uint32_t>((face.vertices.size() - 2) * 3);
    } else {
        outFace.numIndices = 0;
    }

    // Calculate face bounds
    outFace.boundsMin = outFace.boundsMax = face.vertices[0].position;
    for (const auto& vert : face.vertices) {
        outFace.boundsMin = glm::min(outFace.boundsMin, vert.position);
        outFace.boundsMax = glm::max(outFace.boundsMax, vert.position);
    }
}

uint32_t BSPCompiler::GetMaterialIndex(const std::string& materialName) {
    auto it = m_materialMap.find(materialName);
    if (it != m_materialMap.end()) {
        return it->second;
    }

    uint32_t idx = static_cast<uint32_t>(m_bsp->GetMaterials().size());
    BSPMaterial mat;
    mat.name = materialName;
    m_bsp->GetMaterials().push_back(mat);
    m_materialMap[materialName] = idx;

    return idx;
}

uint64_t BSPCompiler::HashPlane(const BSPPlane& plane) const {
    // Quantize plane components to allow for floating point tolerance
    // Use 1000x scale for ~0.001 precision
    auto quantize = [](float v) -> int32_t {
        return static_cast<int32_t>(v * 1000.0f + (v >= 0 ? 0.5f : -0.5f));
    };
    
    int32_t nx = quantize(plane.normal.x);
    int32_t ny = quantize(plane.normal.y);
    int32_t nz = quantize(plane.normal.z);
    int32_t d = quantize(plane.distance);
    
    // Combine into 64-bit hash using FNV-1a style mixing
    uint64_t hash = 14695981039346656037ULL;  // FNV offset basis
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;
    
    hash ^= static_cast<uint64_t>(nx); hash *= FNV_PRIME;
    hash ^= static_cast<uint64_t>(ny); hash *= FNV_PRIME;
    hash ^= static_cast<uint64_t>(nz); hash *= FNV_PRIME;
    hash ^= static_cast<uint64_t>(d);  hash *= FNV_PRIME;
    
    return hash;
}

uint32_t BSPCompiler::GetPlaneIndex(const BSPPlane& plane) {
    // Plane deduplication using hash map
    uint64_t hash = HashPlane(plane);
    
    auto it = m_planeHashMap.find(hash);
    if (it != m_planeHashMap.end()) {
        // Verify it's actually the same plane (handle hash collisions)
        const BSPPlane& existing = m_bsp->GetPlanes()[it->second];
        float normalDiff = glm::length(existing.normal - plane.normal);
        float distDiff = std::abs(existing.distance - plane.distance);
        
        // Also check for flipped plane (opposite normal, negated distance)
        float flippedNormalDiff = glm::length(existing.normal + plane.normal);
        float flippedDistDiff = std::abs(existing.distance + plane.distance);
        
        if ((normalDiff < 0.002f && distDiff < 0.002f) ||
            (flippedNormalDiff < 0.002f && flippedDistDiff < 0.002f)) {
            m_planesReused++;
            return it->second;
        }
    }
    
    // New unique plane
    uint32_t idx = static_cast<uint32_t>(m_bsp->GetPlanes().size());
    m_bsp->GetPlanes().push_back(plane);
    m_planeHashMap[hash] = idx;
    return idx;
}

// ============================================================================
// Collision Hull Generation (Phase 2)
// ============================================================================

void BSPCompiler::GenerateCollisionHulls(const Map& map) {
    if (m_options.verbose) {
        std::cout << "[BSPCompiler] Generating collision hulls..." << std::endl;
    }

    BSPCollision& collision = m_bsp->GetCollision();
    collision.Clear();

    uint32_t brushIndex = 0;
    for (const auto& brush : map.GetBrushes()) {
        // Skip non-collision brushes
        if (!brush.HasCollision()) {
            brushIndex++;
            continue;
        }

        GenerateBrushCollisionHull(brush, brushIndex);
        brushIndex++;
    }

    if (m_options.verbose) {
        std::cout << "[BSPCompiler] Generated " << collision.GetBrushCount() 
                  << " collision brushes with " << collision.GetPlaneCount() << " planes" << std::endl;
    }
}

void BSPCompiler::GenerateBrushCollisionHull(const Brush& brush, uint32_t brushId) {
    // For Phase 2, we only support cube collision hulls
    // Other shapes can be approximated as their bounding box
    switch (brush.shape) {
        case BrushShape::Cube:
            GenerateCubeCollisionHull(brush, brushId);
            break;
        case BrushShape::Sphere:
        case BrushShape::Cylinder:
        case BrushShape::Cone:
        default:
            // For non-cube shapes, use AABB as collision hull
            // This is a simplification - future phases could support more shapes
            GenerateCubeCollisionHull(brush, brushId);
            break;
    }
}

void BSPCompiler::GenerateCubeCollisionHull(const Brush& brush, uint32_t brushId) {
    BSPCollision& collision = m_bsp->GetCollision();
    
    Vec3 pos = brush.position;
    Vec3 halfSize = brush.size * 0.5f;

    // Apply rotation if needed
    Mat4 rotMat = Mat4(1.0f);
    bool hasRotation = (brush.rotation != Vec3(0.0f));
    if (hasRotation) {
        rotMat = glm::rotate(rotMat, glm::radians(brush.rotation.x), Vec3(1, 0, 0));
        rotMat = glm::rotate(rotMat, glm::radians(brush.rotation.y), Vec3(0, 1, 0));
        rotMat = glm::rotate(rotMat, glm::radians(brush.rotation.z), Vec3(0, 0, 1));
    }

    auto transformNormal = [&](const Vec3& n) -> Vec3 {
        if (!hasRotation) return n;
        Vec4 tn = rotMat * Vec4(n, 0.0f);
        return glm::normalize(Vec3(tn));
    };

    // Create collision brush
    BSPCollisionBrush collBrush;
    collBrush.brushId = brushId;
    collBrush.contents = brush.IsTrigger() ? BSPContents::Trigger : BSPContents::Solid;
    collBrush.firstPlane = static_cast<uint32_t>(collision.GetPlanes().size());
    collBrush.numPlanes = 6;
    
    // Glass brushes don't cast shadows
    if (brush.shaderType == ShaderType::Glass) {
        collBrush.noShadow = true;
    }

    // Calculate rotated corners for bounds
    Vec3 corners[8] = {
        Vec3(-halfSize.x, -halfSize.y, -halfSize.z),
        Vec3( halfSize.x, -halfSize.y, -halfSize.z),
        Vec3(-halfSize.x,  halfSize.y, -halfSize.z),
        Vec3( halfSize.x,  halfSize.y, -halfSize.z),
        Vec3(-halfSize.x, -halfSize.y,  halfSize.z),
        Vec3( halfSize.x, -halfSize.y,  halfSize.z),
        Vec3(-halfSize.x,  halfSize.y,  halfSize.z),
        Vec3( halfSize.x,  halfSize.y,  halfSize.z),
    };

    // Transform corners to world space and calculate bounds
    collBrush.boundsMin = Vec3(std::numeric_limits<float>::max());
    collBrush.boundsMax = Vec3(std::numeric_limits<float>::lowest());

    for (int i = 0; i < 8; i++) {
        Vec3 worldCorner;
        if (hasRotation) {
            Vec4 rc = rotMat * Vec4(corners[i], 1.0f);
            worldCorner = pos + Vec3(rc);
        } else {
            worldCorner = pos + corners[i];
        }
        collBrush.boundsMin = glm::min(collBrush.boundsMin, worldCorner);
        collBrush.boundsMax = glm::max(collBrush.boundsMax, worldCorner);
    }

    // Generate 6 planes for the cube (normals pointing outward)
    // Each plane is defined as: normal dot point = distance
    struct PlaneDef {
        Vec3 normal;
        Vec3 point;  // A point on the plane (in local space relative to brush center)
    };

    PlaneDef planes[6] = {
        // +X face
        { Vec3( 1, 0, 0), Vec3( halfSize.x, 0, 0) },
        // -X face
        { Vec3(-1, 0, 0), Vec3(-halfSize.x, 0, 0) },
        // +Y face
        { Vec3( 0, 1, 0), Vec3(0,  halfSize.y, 0) },
        // -Y face
        { Vec3( 0,-1, 0), Vec3(0, -halfSize.y, 0) },
        // +Z face
        { Vec3( 0, 0, 1), Vec3(0, 0,  halfSize.z) },
        // -Z face
        { Vec3( 0, 0,-1), Vec3(0, 0, -halfSize.z) },
    };

    for (int i = 0; i < 6; i++) {
        Vec3 normal = transformNormal(planes[i].normal);
        Vec3 worldPoint;
        if (hasRotation) {
            Vec4 rp = rotMat * Vec4(planes[i].point, 1.0f);
            worldPoint = pos + Vec3(rp);
        } else {
            worldPoint = pos + planes[i].point;
        }

        BSPCollisionPlane collPlane;
        collPlane.normal = normal;
        collPlane.distance = glm::dot(normal, worldPoint);

        collision.GetPlanes().push_back(collPlane);
    }

    collision.GetBrushes().push_back(collBrush);
}

} // namespace Genesis

