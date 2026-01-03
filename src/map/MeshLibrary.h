#pragma once

#include "renderer/mesh/Mesh.h"
#include "renderer/mesh/MeshPrimitives.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace Genesis {

// ============================================================================
// MeshLibrary - Singleton cache for primitive meshes
//
// Prevents creating duplicate meshes for common shapes.
// A cube is a cube - we only need one VBO, and use instancing/transforms.
// ============================================================================
class MeshLibrary {
public:
    static MeshLibrary& Instance() {
        static MeshLibrary instance;
        return instance;
    }

    // ========================================================================
    // Get Primitives (cached)
    // ========================================================================

    // Get a unit cube (-0.5 to 0.5)
    MeshPtr GetCube() {
        return GetOrCreate("__cube", []() {
            return MeshPrimitives::CreateCube("__cube");
        });
    }

    // Get a unit sphere (radius 0.5)
    MeshPtr GetSphere() {
        return GetOrCreate("__sphere", []() {
            return MeshPrimitives::CreateSphere(0.5f, 32, 32, "__sphere");
        });
    }

    // Get a unit plane (1x1 on XZ)
    MeshPtr GetPlane() {
        return GetOrCreate("__plane", []() {
            return MeshPrimitives::CreatePlane(1.0f, 1.0f, "__plane");
        });
    }

    // Get a unit cylinder (radius 0.5, height 1)
    MeshPtr GetCylinder() {
        return GetOrCreate("__cylinder", []() {
            return MeshPrimitives::CreateCylinder(0.5f, 1.0f, 32, "__cylinder");
        });
    }

    // Get a unit cone (radius 0.5, height 1)
    MeshPtr GetCone() {
        return GetOrCreate("__cone", []() {
            return MeshPrimitives::CreateCone(0.5f, 1.0f, 32, "__cone");
        });
    }

    // ========================================================================
    // Generic Access
    // ========================================================================

    // Get mesh by name (returns nullptr if not found)
    MeshPtr Get(const std::string& name) const {
        auto it = m_meshes.find(name);
        return (it != m_meshes.end()) ? it->second : nullptr;
    }

    // Add a custom mesh to the library
    void Add(const std::string& name, MeshPtr mesh) {
        m_meshes[name] = std::move(mesh);
    }

    // Check if mesh exists
    bool Exists(const std::string& name) const {
        return m_meshes.find(name) != m_meshes.end();
    }

    // Remove a mesh
    void Remove(const std::string& name) {
        m_meshes.erase(name);
    }

    // Clear all meshes (except built-in primitives)
    void Clear() {
        m_meshes.clear();
    }

    // Get mesh count
    size_t GetMeshCount() const {
        return m_meshes.size();
    }

    // ========================================================================
    // Mesh Creation by Shape Type
    // ========================================================================

    // Get mesh for a brush shape type
    MeshPtr GetForShape(BrushShape shape) {
        switch (shape) {
            case BrushShape::Cube:     return GetCube();
            case BrushShape::Sphere:   return GetSphere();
            case BrushShape::Cylinder: return GetCylinder();
            case BrushShape::Cone:     return GetCone();
            case BrushShape::Wedge:    return GetCube(); // TODO: Create wedge mesh
            case BrushShape::Custom:   return nullptr;
            default:                   return GetCube();
        }
    }

private:
    MeshLibrary() = default;
    ~MeshLibrary() = default;
    MeshLibrary(const MeshLibrary&) = delete;
    MeshLibrary& operator=(const MeshLibrary&) = delete;

    // Get or create a mesh with lazy initialization
    MeshPtr GetOrCreate(const std::string& name, std::function<MeshPtr()> creator) {
        auto it = m_meshes.find(name);
        if (it != m_meshes.end()) {
            return it->second;
        }
        MeshPtr mesh = creator();
        m_meshes[name] = mesh;
        return mesh;
    }

private:
    std::unordered_map<std::string, MeshPtr> m_meshes;
};

} // namespace Genesis

