#include "MeshPrimitives.h"
#include <cmath>

namespace Genesis {

// ============================================================================
// Cube
// ============================================================================

MeshPtr MeshPrimitives::CreateCube(const std::string& name) {
    return CreateCube(1.0f, name);
}

MeshPtr MeshPrimitives::CreateCube(float size, const std::string& name) {
    return CreateBox(size, size, size, name);
}

MeshPtr MeshPrimitives::CreateBox(float width, float height, float depth, const std::string& name) {
    float w = width * 0.5f;
    float h = height * 0.5f;
    float d = depth * 0.5f;

    MeshBuilder<VertexPNT> builder(name);

    // Front face (+Z)
    uint32_t base = builder.GetVertexCount();
    builder.AddVertex(VertexPNT({-w, -h,  d}, { 0,  0,  1}, {0, 0}));
    builder.AddVertex(VertexPNT({ w, -h,  d}, { 0,  0,  1}, {1, 0}));
    builder.AddVertex(VertexPNT({ w,  h,  d}, { 0,  0,  1}, {1, 1}));
    builder.AddVertex(VertexPNT({-w,  h,  d}, { 0,  0,  1}, {0, 1}));
    builder.AddQuad(base, base+1, base+2, base+3);

    // Back face (-Z)
    base = builder.GetVertexCount();
    builder.AddVertex(VertexPNT({ w, -h, -d}, { 0,  0, -1}, {0, 0}));
    builder.AddVertex(VertexPNT({-w, -h, -d}, { 0,  0, -1}, {1, 0}));
    builder.AddVertex(VertexPNT({-w,  h, -d}, { 0,  0, -1}, {1, 1}));
    builder.AddVertex(VertexPNT({ w,  h, -d}, { 0,  0, -1}, {0, 1}));
    builder.AddQuad(base, base+1, base+2, base+3);

    // Top face (+Y)
    base = builder.GetVertexCount();
    builder.AddVertex(VertexPNT({-w,  h,  d}, { 0,  1,  0}, {0, 0}));
    builder.AddVertex(VertexPNT({ w,  h,  d}, { 0,  1,  0}, {1, 0}));
    builder.AddVertex(VertexPNT({ w,  h, -d}, { 0,  1,  0}, {1, 1}));
    builder.AddVertex(VertexPNT({-w,  h, -d}, { 0,  1,  0}, {0, 1}));
    builder.AddQuad(base, base+1, base+2, base+3);

    // Bottom face (-Y)
    base = builder.GetVertexCount();
    builder.AddVertex(VertexPNT({-w, -h, -d}, { 0, -1,  0}, {0, 0}));
    builder.AddVertex(VertexPNT({ w, -h, -d}, { 0, -1,  0}, {1, 0}));
    builder.AddVertex(VertexPNT({ w, -h,  d}, { 0, -1,  0}, {1, 1}));
    builder.AddVertex(VertexPNT({-w, -h,  d}, { 0, -1,  0}, {0, 1}));
    builder.AddQuad(base, base+1, base+2, base+3);

    // Right face (+X)
    base = builder.GetVertexCount();
    builder.AddVertex(VertexPNT({ w, -h,  d}, { 1,  0,  0}, {0, 0}));
    builder.AddVertex(VertexPNT({ w, -h, -d}, { 1,  0,  0}, {1, 0}));
    builder.AddVertex(VertexPNT({ w,  h, -d}, { 1,  0,  0}, {1, 1}));
    builder.AddVertex(VertexPNT({ w,  h,  d}, { 1,  0,  0}, {0, 1}));
    builder.AddQuad(base, base+1, base+2, base+3);

    // Left face (-X)
    base = builder.GetVertexCount();
    builder.AddVertex(VertexPNT({-w, -h, -d}, {-1,  0,  0}, {0, 0}));
    builder.AddVertex(VertexPNT({-w, -h,  d}, {-1,  0,  0}, {1, 0}));
    builder.AddVertex(VertexPNT({-w,  h,  d}, {-1,  0,  0}, {1, 1}));
    builder.AddVertex(VertexPNT({-w,  h, -d}, {-1,  0,  0}, {0, 1}));
    builder.AddQuad(base, base+1, base+2, base+3);

    return builder.Build();
}

// ============================================================================
// Plane
// ============================================================================

MeshPtr MeshPrimitives::CreatePlane(float width, float depth, const std::string& name) {
    return CreatePlane(width, depth, 1, 1, name);
}

MeshPtr MeshPrimitives::CreatePlane(float width, float depth, uint32_t subdivX, uint32_t subdivZ,
                                    const std::string& name) {
    MeshBuilder<VertexPNT> builder(name);

    float halfW = width * 0.5f;
    float halfD = depth * 0.5f;

    float stepX = width / subdivX;
    float stepZ = depth / subdivZ;
    float uvStepX = 1.0f / subdivX;
    float uvStepZ = 1.0f / subdivZ;

    // Generate vertices
    for (uint32_t z = 0; z <= subdivZ; ++z) {
        for (uint32_t x = 0; x <= subdivX; ++x) {
            float px = -halfW + x * stepX;
            float pz = -halfD + z * stepZ;
            float u = x * uvStepX;
            float v = z * uvStepZ;

            builder.AddVertex(VertexPNT({px, 0, pz}, {0, 1, 0}, {u, v}));
        }
    }

    // Generate indices
    for (uint32_t z = 0; z < subdivZ; ++z) {
        for (uint32_t x = 0; x < subdivX; ++x) {
            uint32_t topLeft = z * (subdivX + 1) + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (z + 1) * (subdivX + 1) + x;
            uint32_t bottomRight = bottomLeft + 1;

            builder.AddQuad(topLeft, bottomLeft, bottomRight, topRight);
        }
    }

    return builder.Build();
}

// ============================================================================
// Sphere
// ============================================================================

MeshPtr MeshPrimitives::CreateSphere(const std::string& name) {
    return CreateSphere(1.0f, 32, 32, name);
}

MeshPtr MeshPrimitives::CreateSphere(float radius, uint32_t rings, uint32_t sectors,
                                     const std::string& name) {
    MeshBuilder<VertexPNT> builder(name);

    const float PI = 3.14159265358979323846f;

    // Generate vertices
    for (uint32_t ring = 0; ring <= rings; ++ring) {
        float phi = PI * ring / rings;  // 0 to PI (pole to pole)
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (uint32_t sector = 0; sector <= sectors; ++sector) {
            float theta = 2.0f * PI * sector / sectors;  // 0 to 2*PI
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            // Position
            float x = sinPhi * cosTheta;
            float y = cosPhi;
            float z = sinPhi * sinTheta;

            Vec3 pos = Vec3(x, y, z) * radius;
            Vec3 normal = Vec3(x, y, z);
            Vec2 uv = Vec2(static_cast<float>(sector) / sectors,
                          static_cast<float>(ring) / rings);

            builder.AddVertex(VertexPNT(pos, normal, uv));
        }
    }

    // Generate indices
    for (uint32_t ring = 0; ring < rings; ++ring) {
        for (uint32_t sector = 0; sector < sectors; ++sector) {
            uint32_t current = ring * (sectors + 1) + sector;
            uint32_t next = current + sectors + 1;

            builder.AddTriangle(current, next, current + 1);
            builder.AddTriangle(current + 1, next, next + 1);
        }
    }

    return builder.Build();
}

// ============================================================================
// Cylinder
// ============================================================================

MeshPtr MeshPrimitives::CreateCylinder(float radius, float height, uint32_t segments,
                                       const std::string& name) {
    MeshBuilder<VertexPNT> builder(name);

    const float PI = 3.14159265358979323846f;
    float halfHeight = height * 0.5f;

    // Side vertices
    for (uint32_t i = 0; i <= segments; ++i) {
        float theta = 2.0f * PI * i / segments;
        float x = std::cos(theta);
        float z = std::sin(theta);
        float u = static_cast<float>(i) / segments;

        // Bottom
        builder.AddVertex(VertexPNT(
            {x * radius, -halfHeight, z * radius},
            {x, 0, z},
            {u, 0}
        ));
        // Top
        builder.AddVertex(VertexPNT(
            {x * radius, halfHeight, z * radius},
            {x, 0, z},
            {u, 1}
        ));
    }

    // Side indices
    for (uint32_t i = 0; i < segments; ++i) {
        uint32_t base = i * 2;
        builder.AddTriangle(base, base + 2, base + 1);
        builder.AddTriangle(base + 1, base + 2, base + 3);
    }

    // Top cap center
    uint32_t topCenter = builder.GetVertexCount();
    builder.AddVertex(VertexPNT({0, halfHeight, 0}, {0, 1, 0}, {0.5f, 0.5f}));

    // Top cap vertices
    uint32_t topStart = builder.GetVertexCount();
    for (uint32_t i = 0; i <= segments; ++i) {
        float theta = 2.0f * PI * i / segments;
        float x = std::cos(theta);
        float z = std::sin(theta);
        builder.AddVertex(VertexPNT(
            {x * radius, halfHeight, z * radius},
            {0, 1, 0},
            {x * 0.5f + 0.5f, z * 0.5f + 0.5f}
        ));
    }

    // Top cap triangles
    for (uint32_t i = 0; i < segments; ++i) {
        builder.AddTriangle(topCenter, topStart + i, topStart + i + 1);
    }

    // Bottom cap center
    uint32_t bottomCenter = builder.GetVertexCount();
    builder.AddVertex(VertexPNT({0, -halfHeight, 0}, {0, -1, 0}, {0.5f, 0.5f}));

    // Bottom cap vertices
    uint32_t bottomStart = builder.GetVertexCount();
    for (uint32_t i = 0; i <= segments; ++i) {
        float theta = 2.0f * PI * i / segments;
        float x = std::cos(theta);
        float z = std::sin(theta);
        builder.AddVertex(VertexPNT(
            {x * radius, -halfHeight, z * radius},
            {0, -1, 0},
            {x * 0.5f + 0.5f, z * 0.5f + 0.5f}
        ));
    }

    // Bottom cap triangles (reversed winding)
    for (uint32_t i = 0; i < segments; ++i) {
        builder.AddTriangle(bottomCenter, bottomStart + i + 1, bottomStart + i);
    }

    return builder.Build();
}

// ============================================================================
// Cone
// ============================================================================

MeshPtr MeshPrimitives::CreateCone(float radius, float height, uint32_t segments,
                                   const std::string& name) {
    MeshBuilder<VertexPNT> builder(name);

    const float PI = 3.14159265358979323846f;
    float halfHeight = height * 0.5f;

    // Apex
    uint32_t apex = builder.GetVertexCount();
    builder.AddVertex(VertexPNT({0, halfHeight, 0}, {0, 1, 0}, {0.5f, 1.0f}));

    // Base vertices for sides
    uint32_t baseStart = builder.GetVertexCount();
    for (uint32_t i = 0; i <= segments; ++i) {
        float theta = 2.0f * PI * i / segments;
        float x = std::cos(theta);
        float z = std::sin(theta);

        // Calculate normal for cone side
        Vec3 tangent = Vec3(-z, 0, x);
        Vec3 toApex = Vec3(0, halfHeight, 0) - Vec3(x * radius, -halfHeight, z * radius);
        Vec3 normal = glm::normalize(glm::cross(tangent, toApex));

        builder.AddVertex(VertexPNT(
            {x * radius, -halfHeight, z * radius},
            normal,
            {static_cast<float>(i) / segments, 0}
        ));
    }

    // Side triangles
    for (uint32_t i = 0; i < segments; ++i) {
        builder.AddTriangle(apex, baseStart + i, baseStart + i + 1);
    }

    // Bottom cap center
    uint32_t bottomCenter = builder.GetVertexCount();
    builder.AddVertex(VertexPNT({0, -halfHeight, 0}, {0, -1, 0}, {0.5f, 0.5f}));

    // Bottom cap vertices
    uint32_t bottomStart = builder.GetVertexCount();
    for (uint32_t i = 0; i <= segments; ++i) {
        float theta = 2.0f * PI * i / segments;
        float x = std::cos(theta);
        float z = std::sin(theta);
        builder.AddVertex(VertexPNT(
            {x * radius, -halfHeight, z * radius},
            {0, -1, 0},
            {x * 0.5f + 0.5f, z * 0.5f + 0.5f}
        ));
    }

    // Bottom cap triangles
    for (uint32_t i = 0; i < segments; ++i) {
        builder.AddTriangle(bottomCenter, bottomStart + i + 1, bottomStart + i);
    }

    return builder.Build();
}

// ============================================================================
// Torus
// ============================================================================

MeshPtr MeshPrimitives::CreateTorus(float outerRadius, float innerRadius,
                                    uint32_t rings, uint32_t sides,
                                    const std::string& name) {
    MeshBuilder<VertexPNT> builder(name);

    const float PI = 3.14159265358979323846f;
    float tubeRadius = (outerRadius - innerRadius) * 0.5f;
    float centerRadius = innerRadius + tubeRadius;

    // Generate vertices
    for (uint32_t ring = 0; ring <= rings; ++ring) {
        float phi = 2.0f * PI * ring / rings;
        float cosPhi = std::cos(phi);
        float sinPhi = std::sin(phi);

        for (uint32_t side = 0; side <= sides; ++side) {
            float theta = 2.0f * PI * side / sides;
            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);

            float x = (centerRadius + tubeRadius * cosTheta) * cosPhi;
            float y = tubeRadius * sinTheta;
            float z = (centerRadius + tubeRadius * cosTheta) * sinPhi;

            Vec3 center = Vec3(centerRadius * cosPhi, 0, centerRadius * sinPhi);
            Vec3 pos = Vec3(x, y, z);
            Vec3 normal = glm::normalize(pos - center);

            Vec2 uv = Vec2(static_cast<float>(ring) / rings,
                          static_cast<float>(side) / sides);

            builder.AddVertex(VertexPNT(pos, normal, uv));
        }
    }

    // Generate indices
    for (uint32_t ring = 0; ring < rings; ++ring) {
        for (uint32_t side = 0; side < sides; ++side) {
            uint32_t current = ring * (sides + 1) + side;
            uint32_t next = current + sides + 1;

            builder.AddTriangle(current, next, current + 1);
            builder.AddTriangle(current + 1, next, next + 1);
        }
    }

    return builder.Build();
}

// ============================================================================
// Fullscreen Quad
// ============================================================================

MeshPtr MeshPrimitives::CreateFullscreenQuad(const std::string& name) {
    MeshBuilder<VertexPT> builder(name);

    // NDC coordinates with UVs
    builder.AddVertex(VertexPT({-1, -1, 0}, {0, 0}));
    builder.AddVertex(VertexPT({ 1, -1, 0}, {1, 0}));
    builder.AddVertex(VertexPT({ 1,  1, 0}, {1, 1}));
    builder.AddVertex(VertexPT({-1,  1, 0}, {0, 1}));

    builder.AddQuad(0, 1, 2, 3);

    return builder.Build();
}

MeshPtr MeshPrimitives::CreateScreenQuad(float x, float y, float width, float height,
                                         const std::string& name) {
    MeshBuilder<VertexPT> builder(name);

    builder.AddVertex(VertexPT({x, y, 0}, {0, 0}));
    builder.AddVertex(VertexPT({x + width, y, 0}, {1, 0}));
    builder.AddVertex(VertexPT({x + width, y + height, 0}, {1, 1}));
    builder.AddVertex(VertexPT({x, y + height, 0}, {0, 1}));

    builder.AddQuad(0, 1, 2, 3);

    return builder.Build();
}

// ============================================================================
// Debug Shapes
// ============================================================================

MeshPtr MeshPrimitives::CreateColoredCube(float size, const Vec3& color,
                                          const std::string& name) {
    MeshBuilder<VertexPC> builder(name);
    float s = size * 0.5f;

    // 8 vertices of a cube
    builder.AddVertex(VertexPC({-s, -s, -s}, color));  // 0
    builder.AddVertex(VertexPC({ s, -s, -s}, color));  // 1
    builder.AddVertex(VertexPC({ s,  s, -s}, color));  // 2
    builder.AddVertex(VertexPC({-s,  s, -s}, color));  // 3
    builder.AddVertex(VertexPC({-s, -s,  s}, color));  // 4
    builder.AddVertex(VertexPC({ s, -s,  s}, color));  // 5
    builder.AddVertex(VertexPC({ s,  s,  s}, color));  // 6
    builder.AddVertex(VertexPC({-s,  s,  s}, color));  // 7

    // 6 faces
    builder.AddQuad(0, 3, 2, 1);  // Back
    builder.AddQuad(4, 5, 6, 7);  // Front
    builder.AddQuad(0, 4, 7, 3);  // Left
    builder.AddQuad(1, 2, 6, 5);  // Right
    builder.AddQuad(3, 7, 6, 2);  // Top
    builder.AddQuad(0, 1, 5, 4);  // Bottom

    return builder.Build();
}

MeshPtr MeshPrimitives::CreateGrid(float size, float spacing, const Vec3& color,
                                   const std::string& name) {
    MeshBuilder<VertexPC> builder(name);
    builder.SetDrawMode(DrawMode::Lines);

    float half = size * 0.5f;
    int count = static_cast<int>(size / spacing);

    for (int i = -count / 2; i <= count / 2; ++i) {
        float pos = i * spacing;

        // Line along X
        builder.AddVertex(VertexPC({-half, 0, pos}, color));
        builder.AddVertex(VertexPC({ half, 0, pos}, color));

        // Line along Z
        builder.AddVertex(VertexPC({pos, 0, -half}, color));
        builder.AddVertex(VertexPC({pos, 0,  half}, color));
    }

    return builder.Build();
}

MeshPtr MeshPrimitives::CreateAxes(float length, const std::string& name) {
    MeshBuilder<VertexPC> builder(name);
    builder.SetDrawMode(DrawMode::Lines);

    // X axis (red)
    builder.AddVertex(VertexPC({0, 0, 0}, {1, 0, 0}));
    builder.AddVertex(VertexPC({length, 0, 0}, {1, 0, 0}));

    // Y axis (green)
    builder.AddVertex(VertexPC({0, 0, 0}, {0, 1, 0}));
    builder.AddVertex(VertexPC({0, length, 0}, {0, 1, 0}));

    // Z axis (blue)
    builder.AddVertex(VertexPC({0, 0, 0}, {0, 0, 1}));
    builder.AddVertex(VertexPC({0, 0, length}, {0, 0, 1}));

    return builder.Build();
}

MeshPtr MeshPrimitives::CreateWireCube(float size, const Vec3& color,
                                       const std::string& name) {
    MeshBuilder<VertexPC> builder(name);
    builder.SetDrawMode(DrawMode::Lines);

    float s = size * 0.5f;

    // Bottom face
    builder.AddVertex(VertexPC({-s, -s, -s}, color));
    builder.AddVertex(VertexPC({ s, -s, -s}, color));

    builder.AddVertex(VertexPC({ s, -s, -s}, color));
    builder.AddVertex(VertexPC({ s, -s,  s}, color));

    builder.AddVertex(VertexPC({ s, -s,  s}, color));
    builder.AddVertex(VertexPC({-s, -s,  s}, color));

    builder.AddVertex(VertexPC({-s, -s,  s}, color));
    builder.AddVertex(VertexPC({-s, -s, -s}, color));

    // Top face
    builder.AddVertex(VertexPC({-s,  s, -s}, color));
    builder.AddVertex(VertexPC({ s,  s, -s}, color));

    builder.AddVertex(VertexPC({ s,  s, -s}, color));
    builder.AddVertex(VertexPC({ s,  s,  s}, color));

    builder.AddVertex(VertexPC({ s,  s,  s}, color));
    builder.AddVertex(VertexPC({-s,  s,  s}, color));

    builder.AddVertex(VertexPC({-s,  s,  s}, color));
    builder.AddVertex(VertexPC({-s,  s, -s}, color));

    // Vertical edges
    builder.AddVertex(VertexPC({-s, -s, -s}, color));
    builder.AddVertex(VertexPC({-s,  s, -s}, color));

    builder.AddVertex(VertexPC({ s, -s, -s}, color));
    builder.AddVertex(VertexPC({ s,  s, -s}, color));

    builder.AddVertex(VertexPC({ s, -s,  s}, color));
    builder.AddVertex(VertexPC({ s,  s,  s}, color));

    builder.AddVertex(VertexPC({-s, -s,  s}, color));
    builder.AddVertex(VertexPC({-s,  s,  s}, color));

    return builder.Build();
}

} // namespace Genesis

