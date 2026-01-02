#pragma once

#include "Mesh.h"
#include "MeshBuilder.h"
#include <memory>

namespace Genesis {

// ============================================================================
// MeshPrimitives - Factory for common geometric shapes
// ============================================================================
class MeshPrimitives {
public:
    // ========================================================================
    // Basic Shapes
    // ========================================================================

    // Create a unit cube centered at origin (-0.5 to 0.5)
    static MeshPtr CreateCube(const std::string& name = "Cube");

    // Create a cube with custom size
    static MeshPtr CreateCube(float size, const std::string& name = "Cube");

    // Create a box with custom dimensions
    static MeshPtr CreateBox(float width, float height, float depth, const std::string& name = "Box");

    // Create a plane on XZ axis (ground plane)
    static MeshPtr CreatePlane(float width, float depth, const std::string& name = "Plane");

    // Create a plane with subdivisions
    static MeshPtr CreatePlane(float width, float depth, uint32_t subdivX, uint32_t subdivZ,
                               const std::string& name = "Plane");

    // Create a UV sphere
    static MeshPtr CreateSphere(float radius, uint32_t rings, uint32_t sectors,
                                const std::string& name = "Sphere");

    // Create a default sphere (radius 1, 32 rings, 32 sectors)
    static MeshPtr CreateSphere(const std::string& name = "Sphere");

    // Create a cylinder
    static MeshPtr CreateCylinder(float radius, float height, uint32_t segments,
                                  const std::string& name = "Cylinder");

    // Create a cone
    static MeshPtr CreateCone(float radius, float height, uint32_t segments,
                              const std::string& name = "Cone");

    // Create a torus (donut)
    static MeshPtr CreateTorus(float outerRadius, float innerRadius,
                               uint32_t rings, uint32_t sides,
                               const std::string& name = "Torus");

    // ========================================================================
    // Screen-Space / 2D Shapes
    // ========================================================================

    // Full-screen quad (for post-processing) - NDC coords (-1 to 1)
    static MeshPtr CreateFullscreenQuad(const std::string& name = "FullscreenQuad");

    // Screen-aligned quad (2D UI)
    static MeshPtr CreateScreenQuad(float x, float y, float width, float height,
                                    const std::string& name = "ScreenQuad");

    // ========================================================================
    // Debug / Wire Shapes (colored, no normals)
    // ========================================================================

    // Colored cube for debug
    static MeshPtr CreateColoredCube(float size, const Vec3& color,
                                     const std::string& name = "ColoredCube");

    // Grid on XZ plane
    static MeshPtr CreateGrid(float size, float spacing, const Vec3& color,
                              const std::string& name = "Grid");

    // Coordinate axes
    static MeshPtr CreateAxes(float length, const std::string& name = "Axes");

    // Wire cube (lines)
    static MeshPtr CreateWireCube(float size, const Vec3& color,
                                  const std::string& name = "WireCube");

private:
    MeshPrimitives() = default;  // Static class
};

} // namespace Genesis

