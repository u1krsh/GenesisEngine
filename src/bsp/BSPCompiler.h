#pragma once

#include "BSPTree.h"
#include "map/Map.h"
#include <vector>
#include <memory>

namespace Genesis {

// ============================================================================
// BSP Compiler - Converts Map brushes into a BSP tree
//
// Phase 1: Simple compilation
// - Convert each brush face to BSP faces
// - Build a simple balanced BSP tree
// - No CSG operations (brushes can overlap)
//
// Future:
// - Phase 2: Proper CSG (subtract brushes from world)
// - Phase 3: Optimal split plane selection
// - Phase 4: PVS calculation
// ============================================================================
class BSPCompiler {
public:
    struct Options {
        bool verbose = true;          // Print progress
        bool balanceTree = true;      // Try to balance the tree
        float splitEpsilon = 0.001f;  // Plane classification epsilon
        uint32_t maxLeafFaces = 32;   // Max faces per leaf before splitting
        uint32_t maxTreeDepth = 64;   // Maximum tree depth
    };

    BSPCompiler() = default;
    ~BSPCompiler() = default;

    // Compile a map into a BSP tree
    BSPTreePtr Compile(const Map& map);
    BSPTreePtr Compile(const Map& map, const Options& options);

    // Get last error
    const std::string& GetLastError() const { return m_lastError; }

private:
    // Intermediate face representation
    struct CompileFace {
        std::vector<BSPVertex> vertices;
        BSPPlane plane;
        uint32_t materialIndex = 0;
        uint32_t brushIndex = 0;

        Vec3 GetCenter() const;
        void CalculateBounds(Vec3& outMin, Vec3& outMax) const;
    };

    // Convert brush to compile faces
    void BrushToFaces(const Brush& brush, uint32_t brushIndex, std::vector<CompileFace>& outFaces);

    // Generate faces for different shapes
    void GenerateCubeFaces(const Brush& brush, uint32_t brushIndex, std::vector<CompileFace>& outFaces);
    void GenerateSphereFaces(const Brush& brush, uint32_t brushIndex, std::vector<CompileFace>& outFaces);
    void GenerateCylinderFaces(const Brush& brush, uint32_t brushIndex, std::vector<CompileFace>& outFaces);
    void GenerateConeFaces(const Brush& brush, uint32_t brushIndex, std::vector<CompileFace>& outFaces);

    // Build BSP tree recursively
    int32_t BuildTree(std::vector<CompileFace>& faces, uint32_t depth);

    // Choose best splitting plane
    uint32_t ChooseSplitPlane(const std::vector<CompileFace>& faces);

    // Split face by plane
    void SplitFace(const CompileFace& face, const BSPPlane& plane,
                   CompileFace& front, CompileFace& back);

    // Classify face against plane
    enum class FaceClassification { Front, Back, OnPlane, Spanning };
    FaceClassification ClassifyFace(const CompileFace& face, const BSPPlane& plane);

    // Create a leaf from faces
    uint32_t CreateLeaf(std::vector<CompileFace>& faces);

    // Triangulate a face and add to BSP
    void TriangulateFace(const CompileFace& face, BSPFace& outFace);

    // Find or add material
    uint32_t GetMaterialIndex(const std::string& materialName);

    // Find or add plane
    uint32_t GetPlaneIndex(const BSPPlane& plane);

private:
    BSPTreePtr m_bsp;
    Options m_options;
    std::string m_lastError;

    // Temporary during compilation
    std::unordered_map<std::string, uint32_t> m_materialMap;

    // Stats
    uint32_t m_currentDepth = 0;
    uint32_t m_maxDepthReached = 0;
};

} // namespace Genesis

