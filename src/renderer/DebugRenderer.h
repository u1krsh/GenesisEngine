#pragma once

#include <vector>
#include <glad/glad.h>

namespace Genesis {

// ============================================================================
// Simple Vertex structure
// ============================================================================
struct Vertex {
    float x, y, z;      // Position
    float r, g, b;      // Color

    Vertex(float px, float py, float pz, float cr = 1.0f, float cg = 1.0f, float cb = 1.0f)
        : x(px), y(py), z(pz), r(cr), g(cg), b(cb) {}
};

// ============================================================================
// DebugRenderer - Renders debug primitives (grid, axes, cubes, etc.)
// ============================================================================
class DebugRenderer {
public:
    DebugRenderer();
    ~DebugRenderer();

    // Non-copyable
    DebugRenderer(const DebugRenderer&) = delete;
    DebugRenderer& operator=(const DebugRenderer&) = delete;

    // Initialize OpenGL resources
    bool Initialize();
    void Shutdown();

    // ========================================================================
    // Immediate mode drawing (rebuilds buffers each frame)
    // ========================================================================

    // Begin a new frame of debug drawing
    void BeginFrame();

    // Add primitives
    void DrawLine(float x1, float y1, float z1, float x2, float y2, float z2,
                  float r, float g, float b);
    void DrawGrid(float size, float spacing, float r = 0.3f, float g = 0.3f, float b = 0.3f);
    void DrawAxes(float length);
    void DrawCube(float x, float y, float z, float size, float r, float g, float b);
    void DrawWireCube(float x, float y, float z, float size, float r, float g, float b);
    void DrawWireBox(float x, float y, float z, float width, float height, float depth, float r, float g, float b);
    void DrawWireSphere(float x, float y, float z, float radius, float r, float g, float b, int segments = 16);
    void DrawWireCone(float x, float y, float z, float radius, float height, float r, float g, float b, int segments = 16);
    void DrawWireCylinder(float x, float y, float z, float radius, float height, float r, float g, float b, int segments = 16);
    void DrawFloor(float size, float y, float r, float g, float b);

    // Render all accumulated primitives
    void RenderLines();
    void RenderTriangles();

    // End frame
    void EndFrame();

private:
    void CreateBuffers();
    void UpdateLineBuffer();
    void UpdateTriangleBuffer();

private:
    // OpenGL objects for lines
    unsigned int m_lineVAO = 0;
    unsigned int m_lineVBO = 0;
    std::vector<Vertex> m_lineVertices;

    // OpenGL objects for triangles (filled shapes)
    unsigned int m_triVAO = 0;
    unsigned int m_triVBO = 0;
    std::vector<Vertex> m_triVertices;

    bool m_initialized = false;
};

} // namespace Genesis

