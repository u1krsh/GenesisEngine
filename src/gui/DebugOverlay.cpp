#include "DebugOverlay.h"
#include "core/Engine.h"
#include "core/Time.h"
#include "renderer/world/StaticWorldRenderer.h"
#include "bsp/BSPRenderer.h"
#include "bsp/BSPBuildVisualizer.h"
#include <sstream>
#include <iomanip>
#include <cfloat>
#include "math/Frustum.h"

namespace Genesis {
namespace GUI {

void DebugOverlay::Render(int screenWidth, int screenHeight) {
    // Check convar OR force visible flag
    auto* showInfo = Console::Instance().FindConVar("ge_showinfo");
    bool showViaConvar = showInfo && showInfo->GetBool();

    if (!showViaConvar && !m_forceVisible) return;

    auto& renderer = GUIRenderer::Instance();
    auto& engine = Engine::Instance();
    auto& camera = engine.GetCamera();
    auto& time = Time::Instance();

    float lineHeight = 14;
    float padding = 8;

    // Calculate panel height based on content
    bool showBSP = BSPRenderer::Instance().IsRenderingActive();
    bool hasPVS = showBSP && BSPRenderer::Instance().HasPVS();
    int baseLines = 20;
    int bspLines = showBSP ? 7 : 0;
    float minimapHeight = hasPVS ? 190.0f : 0.0f;
    int totalLines = baseLines + bspLines;

    // Background panel - positioned at TOP LEFT
    float panelWidth = 280;
    float panelHeight = lineHeight * totalLines + padding * 2 + minimapHeight;
    Rect panelRect(10, 10, panelWidth, panelHeight);

    // Windows 7 style panel with gradient
    renderer.DrawRectGradientV(panelRect, Colors::PanelHeader, Colors::PanelBackground);
    renderer.DrawBorder3D(panelRect, true);

    // Title bar accent line (red)
    renderer.DrawRect(Rect(10, 10, panelWidth, 2), Colors::Accent);

    float x = 10 + padding;
    float y = 10 + padding + 4;

    // Title with red accent
    renderer.DrawText("=== Debug Info ===", x, y, Colors::Accent, 1.0f);
    y += lineHeight + 6;

    // Separator line
    renderer.DrawRect(Rect(x, y - 2, panelWidth - padding * 2, 1), Colors::BorderDark);

    // FPS
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "FPS: " << time.GetFPS();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Frame time
    oss.str("");
    oss << "Frame Time: " << (time.GetDeltaTime() * 1000.0) << " ms";
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Total time
    oss.str("");
    oss << "Total Time: " << std::setprecision(1) << time.GetTotalTime() << " s";
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Frame count
    oss.str("");
    oss << "Frame: " << time.GetFrameCount();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight + 4;

    // Camera section header
    renderer.DrawText("-- Camera --", x, y, Colors::AccentLight, 1.0f);
    y += lineHeight;

    // Camera position
    const auto& pos = camera.GetPosition();
    oss.str("");
    oss << std::setprecision(2);
    oss << "Pos: (" << pos.x << ", " << pos.y << ", " << pos.z << ")";
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Camera angles
    oss.str("");
    oss << "Yaw: " << std::setprecision(1) << camera.GetYaw()
        << "  Pitch: " << camera.GetPitch();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Camera forward vector
    const auto& fwd = camera.GetForward();
    oss.str("");
    oss << std::setprecision(2);
    oss << "Fwd: (" << fwd.x << ", " << fwd.y << ", " << fwd.z << ")";
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight + 4;

    // Settings section header
    renderer.DrawText("-- Settings --", x, y, Colors::AccentLight, 1.0f);
    y += lineHeight;

    // FOV
    oss.str("");
    oss << "FOV: " << std::setprecision(0) << camera.GetFOV();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Time scale
    oss.str("");
    oss << "Time Scale: " << std::setprecision(2) << time.GetTimeScale();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight + 4;

    // Render stats section header
    renderer.DrawText("-- Render Stats --", x, y, Colors::AccentLight, 1.0f);
    y += lineHeight;

    // Get stats from StaticWorldRenderer
    auto& worldRenderer = StaticWorldRenderer::Instance();

    // Objects and draw calls
    oss.str("");
    oss << "Objects: " << worldRenderer.GetObjectsRendered();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    oss.str("");
    oss << "Draw Calls: " << worldRenderer.GetDrawCalls();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Vertex count
    oss.str("");
    uint32_t vertices = worldRenderer.GetVerticesRendered();
    if (vertices > 1000) {
        oss << "Vertices: " << std::setprecision(1) << (vertices / 1000.0f) << "K";
    } else {
        oss << "Vertices: " << vertices;
    }
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Triangle count (polygons)
    oss.str("");
    uint32_t triangles = worldRenderer.GetTrianglesRendered();
    if (triangles > 1000) {
        oss << "Polygons: " << std::setprecision(1) << (triangles / 1000.0f) << "K";
    } else {
        oss << "Polygons: " << triangles;
    }
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Material switches
    oss.str("");
    oss << "Material Switches: " << worldRenderer.GetMaterialSwitches();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Total world objects
    oss.str("");
    oss << "World Objects: " << worldRenderer.GetObjectCount();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // BSP Stats section (only shown when BSP rendering is active)
    if (showBSP) {
        y += 4;
        renderer.DrawText("-- BSP Stats --", x, y, Colors::AccentLight, 1.0f);
        y += lineHeight;

        auto stats = BSPRenderer::Instance().GetStats();

        oss.str("");
        oss << "BSP Nodes: " << stats.numNodes;
        renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
        y += lineHeight;

        oss.str("");
        oss << "BSP Leafs: " << stats.numLeafs;
        renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
        y += lineHeight;

        oss.str("");
        oss << "BSP Faces: " << stats.numFaces;
        renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
        y += lineHeight;

        // Rendered stats - shows culling in action
        uint32_t renderedLeafs = BSPRenderer::Instance().GetRenderedLeafs();
        uint32_t renderedFaces = BSPRenderer::Instance().GetRenderedFaces();
        
        oss.str("");
        oss << "Rendered Leafs: " << renderedLeafs << "/" << stats.numLeafs;
        renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
        y += lineHeight;

        oss.str("");
        oss << "Rendered Faces: " << renderedFaces << "/" << stats.numFaces;
        renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
        y += lineHeight;

        // Calculate and show culling percentage
        float cullPercent = 0.0f;
        if (stats.numLeafs > 0) {
            cullPercent = (1.0f - (float)renderedLeafs / (float)stats.numLeafs) * 100.0f;
        }
        oss.str("");
        oss << "Culled: " << std::fixed << std::setprecision(1) << cullPercent << "%";
        Vec4 cullColor = (cullPercent > 50.0f) ? Vec4(0.0f, 1.0f, 0.3f, 1.0f) : Vec4(1.0f, 0.8f, 0.0f, 1.0f);
        renderer.DrawText(oss.str(), x, y, cullColor, 1.0f);
        y += lineHeight;

        oss.str("");
        oss << "Tree Depth: " << stats.maxTreeDepth;
        renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
        y += lineHeight;

        // ====================================================================
        // PVS Minimap - Top-down view of visible/culled leafs
        // ====================================================================
        auto bsp = BSPRenderer::Instance().GetBSP();
        if (bsp && bsp->HasPVS()) {
            y += 8;
            renderer.DrawText("-- PVS Minimap --", x, y, Colors::AccentLight, 1.0f);
            y += lineHeight;

            const auto& leafs = bsp->GetLeafs();
            const auto& pvs = bsp->GetPVS();

            // Find world bounds (min/max of all leafs)
            Vec3 worldMin(FLT_MAX), worldMax(-FLT_MAX);
            for (const auto& leaf : leafs) {
                if (leaf.numFaces == 0) continue;
                worldMin.x = std::min(worldMin.x, leaf.boundsMin.x);
                worldMin.z = std::min(worldMin.z, leaf.boundsMin.z);
                worldMax.x = std::max(worldMax.x, leaf.boundsMax.x);
                worldMax.z = std::max(worldMax.z, leaf.boundsMax.z);
            }

            // Minimap dimensions
            float mapSize = 150.0f;
            float mapX = x;
            float mapY = y;

            // Draw minimap background
            renderer.DrawRect(Rect(mapX, mapY, mapSize, mapSize), Vec4(0.1f, 0.1f, 0.15f, 0.9f));
            renderer.DrawRectOutline(Rect(mapX, mapY, mapSize, mapSize), Colors::BorderLight, 1.0f);

            // Scale factors
            float worldWidth = worldMax.x - worldMin.x;
            float worldDepth = worldMax.z - worldMin.z;
            if (worldWidth < 0.1f) worldWidth = 1.0f;
            if (worldDepth < 0.1f) worldDepth = 1.0f;
            float scale = (mapSize - 4.0f) / std::max(worldWidth, worldDepth);

            // Get camera info
            Vec3 camPos = engine.GetCamera().GetPosition();
            int32_t cameraLeaf = bsp->FindLeaf(camPos);

            // Build frustum for runtime visibility check
            Frustum frustum;
            Mat4 vp = engine.GetCamera().GetProjectionMatrix() * engine.GetCamera().GetViewMatrix();
            frustum.Update(vp);

            // Determine visibility using RUNTIME frustum check (not static PVS)
            std::vector<bool> isVisible(leafs.size(), false);
            for (size_t i = 0; i < leafs.size(); ++i) {
                const auto& leaf = leafs[i];
                if (leaf.numFaces == 0) continue;
                // Check if leaf is in camera frustum
                isVisible[i] = frustum.IsBoxVisible(leaf.boundsMin, leaf.boundsMax);
            }

            // Draw each leaf
            for (size_t i = 0; i < leafs.size(); ++i) {
                const auto& leaf = leafs[i];
                if (leaf.numFaces == 0) continue;

                // Map world coords to minimap coords (XZ plane)
                float lx = mapX + 2.0f + (leaf.boundsMin.x - worldMin.x) * scale;
                float lz = mapY + 2.0f + (leaf.boundsMin.z - worldMin.z) * scale;
                float lw = (leaf.boundsMax.x - leaf.boundsMin.x) * scale;
                float lh = (leaf.boundsMax.z - leaf.boundsMin.z) * scale;

                // Clamp minimum size for visibility
                if (lw < 2.0f) lw = 2.0f;
                if (lh < 2.0f) lh = 2.0f;

                // Color based on visibility
                Vec4 color;
                if (static_cast<int32_t>(i) == cameraLeaf) {
                    color = Vec4(0.0f, 1.0f, 0.0f, 0.8f);  // Green - current
                } else if (isVisible[i]) {
                    color = Vec4(0.0f, 0.7f, 1.0f, 0.6f);  // Cyan - visible
                } else {
                    color = Vec4(1.0f, 0.2f, 0.2f, 0.4f);  // Red - culled
                }

                renderer.DrawRect(Rect(lx, lz, lw, lh), color);
            }

            // Draw camera position
            float camMapX = mapX + 2.0f + (camPos.x - worldMin.x) * scale;
            float camMapZ = mapY + 2.0f + (camPos.z - worldMin.z) * scale;
            renderer.DrawRect(Rect(camMapX - 3, camMapZ - 3, 6, 6), Vec4(1.0f, 1.0f, 0.0f, 1.0f));

            y += mapSize + 4;

            // Legend
            renderer.DrawRect(Rect(x, y, 10, 10), Vec4(0.0f, 1.0f, 0.0f, 0.8f));
            renderer.DrawText("Current", x + 14, y, Colors::Text, 0.9f);
            renderer.DrawRect(Rect(x + 70, y, 10, 10), Vec4(0.0f, 0.7f, 1.0f, 0.6f));
            renderer.DrawText("Visible", x + 84, y, Colors::Text, 0.9f);
            renderer.DrawRect(Rect(x + 140, y, 10, 10), Vec4(1.0f, 0.2f, 0.2f, 0.4f));
            renderer.DrawText("Culled", x + 154, y, Colors::Text, 0.9f);
        }
    }

    // ========================================================================
    // BSP Build Visualization Panel (shows when playing)
    // ========================================================================
    auto& visualizer = BSPBuildVisualizer::Instance();
    if (visualizer.GetTotalSteps() > 0) {
        // Draw visualization panel on the right side
        float vizPanelWidth = 300.0f;
        float vizPanelHeight = 320.0f;
        float vizPanelX = screenWidth - vizPanelWidth - 10;
        float vizPanelY = 10;

        // Panel background
        renderer.DrawRect(Rect(vizPanelX, vizPanelY, vizPanelWidth, vizPanelHeight), 
                         Vec4(0.05f, 0.05f, 0.1f, 0.95f));
        renderer.DrawRectOutline(Rect(vizPanelX, vizPanelY, vizPanelWidth, vizPanelHeight), 
                                Colors::AccentLight, 1.0f);

        float vx = vizPanelX + 8;
        float vy = vizPanelY + 8;

        // Title
        renderer.DrawText("=== BSP Node Builder ===", vx, vy, Colors::Accent, 1.0f);
        vy += 18;

        // Status
        std::ostringstream vizOss;
        vizOss << "Step: " << visualizer.GetCurrentStep() << " / " << visualizer.GetTotalSteps();
        renderer.DrawText(vizOss.str(), vx, vy, Colors::Text, 1.0f);
        vy += 14;

        renderer.DrawText(visualizer.IsPlaying() ? "Status: Playing..." : "Status: Paused (F7 to play)", 
                         vx, vy, visualizer.IsPlaying() ? Colors::AccentLight : Colors::TextDim, 1.0f);
        vy += 18;

        // Map view
        float mapX = vx;
        float mapY = vy;
        float mapSize = vizPanelWidth - 16;
        Rect mapRect(mapX, mapY, mapSize, mapSize);

        // Draw map background (Black)
        renderer.DrawRect(mapRect, Vec4(0.0f, 0.0f, 0.0f, 1.0f));
        renderer.DrawRectOutline(mapRect, Vec4(0.5f, 0.5f, 0.5f, 1.0f), 1.0f);

        // Clip content to map area
        renderer.PushClipRect(mapRect);

        // Draw leafs up to current step
        Vec3 worldMin = visualizer.GetWorldMin();
        Vec3 worldMax = visualizer.GetWorldMax();
        float worldWidth = worldMax.x - worldMin.x;
        float worldDepth = worldMax.z - worldMin.z;
        if (worldWidth < 0.1f) worldWidth = 1.0f;
        if (worldDepth < 0.1f) worldDepth = 1.0f;
        
        // Calculate scale and centering offset
        float padding = 10.0f;
        float scale = (mapSize - padding * 2.0f) / std::max(worldWidth, worldDepth);
        float offsetX = (mapSize - worldWidth * scale) * 0.5f;
        float offsetZ = (mapSize - worldDepth * scale) * 0.5f;

        // Draw static map geometry (White lines)
        const auto& staticLines = visualizer.GetStaticLines();
        Vec4 staticColor = Vec4(1.0f, 1.0f, 1.0f, 0.5f);
        
        // Optimizing draw calls for lines is ideal, but passing one by one is fine for debug overlay
        for (const auto& line : staticLines) {
             float lx1 = mapX + offsetX + (line.start.x - worldMin.x) * scale;
             float lz1 = mapY + offsetZ + (line.start.z - worldMin.z) * scale;
             float lx2 = mapX + offsetX + (line.end.x - worldMin.x) * scale;
             float lz2 = mapY + offsetZ + (line.end.z - worldMin.z) * scale;
             
             renderer.DrawLine(Vec2(lx1, lz1), Vec2(lx2, lz2), staticColor, 1.0f);
        }

        const auto& steps = visualizer.GetSteps();
        size_t currentStep = visualizer.GetCurrentStep();

        for (size_t i = 0; i < currentStep && i < steps.size(); ++i) {
            const auto& step = steps[i];
            
            // Only draw the CURRENT split line (Red)
            // No leaf filling, no old history stacking, as per user request ("animation is in layers... I want one layer")
            
            if (step.type == BSPBuildStep::Type::SplitPlane && i == currentStep - 1) {
                // Draw split plane as a line (if vertical-ish)
                if (std::abs(step.planeNormal.y) < 0.9f) {
                    // It's a wall-like split
                    Vec2 center = { (step.boundsMin.x + step.boundsMax.x) * 0.5f, 
                                    (step.boundsMin.z + step.boundsMax.z) * 0.5f };
                                    
                    // Project simple line through center perpendicular to normal
                    // Make it cover the map size to ensure it cuts through everything visually
                    float lineLen = mapSize / scale * 1.5f; 
                    Vec2 tang = { -step.planeNormal.z, step.planeNormal.x }; // Tangent
                    
                    // Center of split is roughly the plane point projected
                    float cx = mapX + offsetX + (step.planePoint.x - worldMin.x) * scale;
                    float cz = mapY + offsetZ + (step.planePoint.z - worldMin.z) * scale;
                    
                    float dx = tang.x * lineLen * scale * 0.5f;
                    float dz = tang.y * lineLen * scale * 0.5f;
                    
                    // Draw split line (Red for active split)
                    Vec4 splitColor = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
                    float thickness = 2.0f;
                    
                    Vec2 pStart(cx - dx, cz - dz);
                    Vec2 pEnd(cx + dx, cz + dz);
                    
                    renderer.DrawLine(pStart, pEnd, splitColor, thickness);
                }
            }
        }
        
        renderer.PopClipRect();
    }
}

} // namespace GUI
} // namespace Genesis
