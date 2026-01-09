// ============================================================================
// OracularV2 Viewport Layout Implementation
// ============================================================================

#include "ViewportLayout.h"
#include "Grid.h"
#include "SelectionManager.h"
#include "Gizmo.h"
#include "tools/BlockTool.h"
#include "EditorBrush.h"
#include "EditorEntity.h"
#include "map/Map.h"

#include <imgui.h>
#include <iostream>
#include <glm/gtx/rotate_vector.hpp>

// ============================================================================
// Constructor/Destructor
// ============================================================================

ViewportLayout::ViewportLayout() {
    // Create viewports
    m_viewports[0] = std::make_unique<Viewport>(ViewportType::Perspective3D);
    m_viewports[1] = std::make_unique<Viewport>(ViewportType::TopXZ);
    m_viewports[2] = std::make_unique<Viewport>(ViewportType::FrontXY);
    m_viewports[3] = std::make_unique<Viewport>(ViewportType::SideYZ);
}

ViewportLayout::~ViewportLayout() = default;

// ============================================================================
// Initialization
// ============================================================================

void ViewportLayout::Initialize(int totalWidth, int totalHeight) {
    int halfWidth = totalWidth / 2;
    int halfHeight = totalHeight / 2;
    
    for (auto& viewport : m_viewports) {
        viewport->Initialize(halfWidth, halfHeight);
    }
    
    m_initialized = true;
}

// ============================================================================
// Render
// ============================================================================

void ViewportLayout::Render(Grid* grid, Genesis::Map* map,
                            std::vector<EditorBrush>* brushes,
                            std::vector<EditorEntity>* entities,
                            SelectionManager* selection,
                            Gizmo* gizmo,
                            BlockTool* blockTool,
                            EditorTool currentTool,
                            EntityPaletteType entityType) {
    const char* viewportNames[] = {"3D Perspective", "Top View", "Front View", "Side View"};
    
    for (int i = 0; i < 4; i++) {
        ImGui::Begin(viewportNames[i]);
        
        ImVec2 windowSize = ImGui::GetContentRegionAvail();
        int newWidth = static_cast<int>(windowSize.x);
        int newHeight = static_cast<int>(windowSize.y);
        
        // Resize viewport if needed
        if (newWidth > 0 && newHeight > 0) {
            if (m_viewports[i]->GetWidth() != newWidth || 
                m_viewports[i]->GetHeight() != newHeight) {
                // First time? Initialize instead of just resize
                if (m_viewports[i]->GetWidth() == 0) {
                    m_viewports[i]->Initialize(newWidth, newHeight);
                } else {
                    m_viewports[i]->Resize(newWidth, newHeight);
                }
            }
        }
        
        // Render viewport content
        m_viewports[i]->BeginRender();
        m_viewports[i]->RenderGrid(grid);
        m_viewports[i]->RenderBrushes(brushes, selection);
        m_viewports[i]->RenderEntities(entities, selection);
        
        // Render block tool preview
        if (blockTool && blockTool->HasPreview()) {
            m_viewports[i]->RenderBrushPreview(blockTool->GetPreview(), grid);
        }
        
        // Render gizmo for selection
        if (selection && selection->HasSelection() && currentTool == EditorTool::Select) {
            Genesis::Vec3 center = selection->GetSelectionCenter();
            if (gizmo) {
                gizmo->SetPosition(center);
                m_viewports[i]->RenderGizmo(gizmo);
            }
        }
        
        m_viewports[i]->EndRender();
        
        // Display viewport texture
        unsigned int textureId = m_viewports[i]->GetFramebufferTexture();
        if (textureId != 0 && windowSize.x > 0 && windowSize.y > 0) {
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            
            // Flip Y for correct orientation
            ImGui::Image(
                (ImTextureID)(intptr_t)textureId, 
                windowSize,
                ImVec2(0, 1), ImVec2(1, 0)
            );
            
            // Handle viewport input when hovered OR if this is the active viewport and we are transforming
            bool isTransforming = (m_transform.mode != TransformMode::None && m_activeViewport == i);
            
            if (ImGui::IsItemHovered() || isTransforming) {
                if (ImGui::IsItemHovered()) m_activeViewport = i;
                
                // Handle input
                HandleViewportInput(m_viewports[i].get(), i, grid, 
                                   brushes, entities, selection, gizmo, blockTool, 
                                   currentTool, entityType);
            }
            
            // Draw box selection rectangle overlay
            if (m_boxSelect.active && m_boxSelect.viewportIndex == i) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 p1(cursorPos.x + m_boxSelect.startPos.x, cursorPos.y + m_boxSelect.startPos.y);
                ImVec2 p2(cursorPos.x + m_boxSelect.endPos.x, cursorPos.y + m_boxSelect.endPos.y);
                drawList->AddRectFilled(p1, p2, IM_COL32(50, 100, 200, 50));
                drawList->AddRect(p1, p2, IM_COL32(100, 150, 255, 200), 0.0f, 0, 2.0f);
            }
            
            // Draw transform mode status overlay
            if (m_transform.mode != TransformMode::None) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                const char* modeText = "";
                switch (m_transform.mode) {
                    case TransformMode::Translate: modeText = "GRAB"; break;
                    case TransformMode::Rotate: modeText = "ROTATE"; break;
                    case TransformMode::Scale: modeText = "SCALE"; break;
                    default: break;
                }
                
                const char* axisText = "";
                ImU32 axisColor = IM_COL32(255, 255, 255, 255);
                switch (m_transform.axis) {
                    case TransformAxis::X: axisText = " X"; axisColor = IM_COL32(255, 100, 100, 255); break;
                    case TransformAxis::Y: axisText = " Y"; axisColor = IM_COL32(100, 255, 100, 255); break;
                    case TransformAxis::Z: axisText = " Z"; axisColor = IM_COL32(100, 100, 255, 255); break;
                    default: break;
                }
                
                ImVec2 textPos(cursorPos.x + 10, cursorPos.y + 10);
                drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), modeText);
                if (m_transform.axis != TransformAxis::None) {
                    ImVec2 axisTextPos(textPos.x + ImGui::CalcTextSize(modeText).x, textPos.y);
                    drawList->AddText(axisTextPos, axisColor, axisText);
                }
            }
        }
        
        ImGui::End();
    }
}

// ============================================================================
// Input Handling
// ============================================================================

void ViewportLayout::HandleViewportInput(Viewport* viewport, int viewportIndex,
                                          Grid* grid,
                                          std::vector<EditorBrush>* brushes,
                                          std::vector<EditorEntity>* entities,
                                          SelectionManager* selection,
                                          Gizmo* gizmo,
                                          BlockTool* blockTool,
                                          EditorTool currentTool,
                                          EntityPaletteType entityType) {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = io.MousePos;
    ImVec2 itemMin = ImGui::GetItemRectMin();
    float localX = mousePos.x - itemMin.x;
    float localY = mousePos.y - itemMin.y;
    
    // ------------------------------------------------------------------------
    // Transform Modal Logic (Blocks other input)
    // ------------------------------------------------------------------------
    if (m_transform.mode != TransformMode::None) {
        UpdateTransform(Genesis::Vec2(localX, localY), viewport);
        
        // Axis Constraints
        if (ImGui::IsKeyPressed(ImGuiKey_X)) m_transform.axis = (m_transform.axis == TransformAxis::X) ? TransformAxis::None : TransformAxis::X;
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) m_transform.axis = (m_transform.axis == TransformAxis::Y) ? TransformAxis::None : TransformAxis::Y;
        if (ImGui::IsKeyPressed(ImGuiKey_Z)) m_transform.axis = (m_transform.axis == TransformAxis::Z) ? TransformAxis::None : TransformAxis::Z;
        
        // Confirm/Cancel
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) ApplyTransform();
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsKeyPressed(ImGuiKey_Escape)) CancelTransform();
        
        return; // Consume input
    }

    // Trigger Transforms (G, R, S) - only when Select tool is active
    if (currentTool == EditorTool::Select) {
        if (ImGui::IsKeyPressed(ImGuiKey_G)) {
             int count = selection->GetSelectedCount();
             std::cout << "[DEBUG] G key pressed! Selection count: " << count << " ActiveViewport: " << m_activeViewport << "\n";
             if (count > 0) {
                 StartTransform(TransformMode::Translate, selection, Genesis::Vec2(localX, localY));
             } else {
                 std::cout << "[DEBUG] Cannot Grab: No selection.\n";
             }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R)) StartTransform(TransformMode::Rotate, selection, Genesis::Vec2(localX, localY));
        if (ImGui::IsKeyPressed(ImGuiKey_S) && !io.KeyCtrl) StartTransform(TransformMode::Scale, selection, Genesis::Vec2(localX, localY));
    }
    
    // ------------------------------------------------------------------------
    // Blender-style Navigation
    // ------------------------------------------------------------------------
    
    // Middle Mouse Drag: Orbit (Default) or Pan (Shift) or Zoom (Ctrl)
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        ImVec2 delta = io.MouseDelta;
        
        if (io.KeyShift) {
            // Shift + MMB = Pan
            viewport->Pan(delta.x * 0.5f, delta.y * 0.5f);
        } else if (io.KeyCtrl) {
            // Ctrl + MMB = Zoom (Vertical drag)
            viewport->Zoom(delta.y * 0.1f);
        } else {
            // MMB = Orbit (3D) or Pan (2D)
            if (viewport->IsPerspective()) {
                viewport->Orbit(delta.x, delta.y);
            } else {
                viewport->Pan(delta.x * 0.5f, delta.y * 0.5f);
            }
        }
    }
    
    // Mouse wheel zoom (Zoom towards cursor)
    if (io.MouseWheel != 0) {
        viewport->ZoomAtPoint(io.MouseWheel, localX, localY);
    }

    // Numpad View Switching
    if (ImGui::IsWindowFocused()) {
        if (ImGui::IsKeyPressed(ImGuiKey_Keypad7)) {
            viewport->SetType(ViewportType::TopXZ);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Keypad1)) {
            viewport->SetType(ViewportType::FrontXY);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Keypad3)) {
            viewport->SetType(ViewportType::SideYZ);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Keypad5)) {
            // Toggle Perspective/Ortho
            if (viewport->IsPerspective()) {
                 // Switch to nearest ortho view? Or specific perspective?
                 // For now, let's just make it TopXZ as default ortho
                 viewport->SetType(ViewportType::TopXZ);
            } else {
                 viewport->SetType(ViewportType::Perspective3D);
            }
        }
    }
    
    // WASD Movement (3D only)
    if (viewport->IsPerspective()) {
        float speed = 500.0f * io.DeltaTime; // Default speed
        if (io.KeyShift) speed *= 2.5f;      // Sprint speed
        
        Genesis::Vec3 forward = viewport->GetCamera().GetForward();
        Genesis::Vec3 right = glm::cross(forward, Genesis::Vec3(0, 1, 0));
        
        // Flatten vectors for generic movement on XZ plane (FPS style)
        // Or keep 3D for "spectator" style. Let's do Camera-relative planar movement for now.
        // Actually, for map editor, usually we want to move the pivot.
        
        Genesis::Vec3 moveDelta(0.0f);
        
        if (ImGui::IsKeyDown(ImGuiKey_W)) moveDelta += forward * speed;
        if (ImGui::IsKeyDown(ImGuiKey_S)) moveDelta -= forward * speed;
        if (ImGui::IsKeyDown(ImGuiKey_D)) moveDelta += right * speed;
        if (ImGui::IsKeyDown(ImGuiKey_A)) moveDelta -= right * speed;
        
        if (glm::length(moveDelta) > 0.0f) {
            viewport->MoveOrbitTarget(moveDelta);
        }
    }
    
    // ------------------------------------------------------------------------
    // Focus on selection (F key)
    // ------------------------------------------------------------------------
    if (ImGui::IsKeyPressed(ImGuiKey_F) && selection && selection->HasSelection()) {
        Genesis::Vec3 center = selection->GetSelectionCenter();
        viewport->FocusOn(center);
    }
    
    // Home key resets view
    if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
        viewport->FocusOn(Genesis::Vec3(0.0f));
    }
    
    // ------------------------------------------------------------------------
    // Hover highlighting
    // ------------------------------------------------------------------------
    if (brushes && currentTool == EditorTool::Select && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        // Clear all hover states first
        for (auto& brush : *brushes) {
            brush.isHovered = false;
        }
        
        // Find brush under cursor and highlight it
        EditorBrush* hoveredBrush = RaycastBrush(viewport, localX, localY, brushes);
        if (hoveredBrush) {
            hoveredBrush->isHovered = true;
        }
    }
    
    // ------------------------------------------------------------------------
    // Shift+D Duplicate
    // ------------------------------------------------------------------------
    if (ImGui::IsKeyPressed(ImGuiKey_D) && io.KeyShift && selection && selection->HasSelection() && brushes) {
        // Duplicate selected brushes
        std::vector<EditorBrush*> newBrushes;
        for (auto* brush : selection->GetSelectedBrushes()) {
            EditorBrush copy = *brush;
            copy.editorId = 0; // Will be assigned when added
            copy.isSelected = false;
            copy.brush.position += Genesis::Vec3(32, 0, 32); // Offset duplicate
            brushes->push_back(copy);
            newBrushes.push_back(&brushes->back());
        }
        
        // Select the new duplicates
        selection->ClearSelection();
        for (auto* b : newBrushes) {
            selection->Select(b, true);
        }
        
        // Start transform immediately
        StartTransform(TransformMode::Translate, selection, Genesis::Vec2(localX, localY));
    }
    
    // Left click handling based on tool
    if (currentTool == EditorTool::Select) {
        // Selection tool
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            Genesis::Vec2 clickPos(localX, localY);
            double currentTime = ImGui::GetTime();
            
            // Check if this is a repeat click at same location for click-through
            bool isSameSpot = (glm::distance(clickPos, m_clickThrough.lastClickPos) < 5.0f &&
                               viewportIndex == m_clickThrough.viewportIndex &&
                               (currentTime - m_clickThrough.lastClickTime) < m_clickThrough.clickTimeout);
            
            if (isSameSpot) {
                m_clickThrough.cycleIndex++;
            } else {
                m_clickThrough.cycleIndex = 0;
            }
            
            m_clickThrough.lastClickPos = clickPos;
            m_clickThrough.viewportIndex = viewportIndex;
            m_clickThrough.lastClickTime = currentTime;
            
            // Collect all objects under cursor sorted by distance
            // Use a variant to store both brushes and entities
            Genesis::Ray ray = viewport->ScreenToWorldRay(localX, localY);
            
            struct HitObject {
                float distance;
                bool isBrush;
                void* ptr;  // EditorBrush* or EditorEntity*
            };
            std::vector<HitObject> allHits;
            
            // Collect brushes
            if (brushes) {
                for (auto& brush : *brushes) {
                    if (!brush.isVisible) continue;
                    Genesis::AABB aabb = brush.GetAABB();
                    float t = ray.IntersectAABB(aabb);
                    if (t > 0) {
                        allHits.push_back({t, true, &brush});
                    }
                }
            }
            
            // Collect entities
            if (entities) {
                for (auto& entity : *entities) {
                    if (!entity.isVisible) continue;
                    // Reduced radius from 16.0f to 5.0f for more precise selection
                    // Prevents accidental selection when clicking "off" (empty space)
                    float radius = 5.0f;
                    Genesis::AABB aabb(
                        entity.entity.position - Genesis::Vec3(radius),
                        entity.entity.position + Genesis::Vec3(radius)
                    );
                    float t = ray.IntersectAABB(aabb);
                    if (t > 0) {
                        allHits.push_back({t, false, &entity});
                    }
                }
            }
            
            // Sort by distance (front to back)
            std::sort(allHits.begin(), allHits.end(),
                [](const HitObject& a, const HitObject& b) { return a.distance < b.distance; });
            
            bool addToSelection = io.KeyCtrl || io.KeyShift;
            
            if (!allHits.empty()) {
                int idx = m_clickThrough.cycleIndex % (int)allHits.size();
                HitObject& hit = allHits[idx];
                
                if (hit.isBrush) {
                    EditorBrush* hitBrush = static_cast<EditorBrush*>(hit.ptr);
                    if (addToSelection) {
                        selection->ToggleSelection(hitBrush);
                    } else {
                        selection->Select(hitBrush, false);
                    }
                } else {
                    EditorEntity* hitEntity = static_cast<EditorEntity*>(hit.ptr);
                    if (!addToSelection) {
                        selection->ClearSelection();
                    }
                    selection->SelectEntity(hitEntity, addToSelection);
                }
            } else if (!addToSelection) {
                // No hit - start box selection
                m_boxSelect.active = true;
                m_boxSelect.startPos = Genesis::Vec2(localX, localY);
                m_boxSelect.endPos = m_boxSelect.startPos;
                m_boxSelect.viewportIndex = viewportIndex;
                selection->ClearSelection();
            }
        }
        
        // Update box selection while dragging
        if (m_boxSelect.active && m_boxSelect.viewportIndex == viewportIndex) {
            m_boxSelect.endPos = Genesis::Vec2(localX, localY);
            
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                // Finish box selection - select all brushes in the box
                float minX = std::min(m_boxSelect.startPos.x, m_boxSelect.endPos.x);
                float maxX = std::max(m_boxSelect.startPos.x, m_boxSelect.endPos.x);
                float minY = std::min(m_boxSelect.startPos.y, m_boxSelect.endPos.y);
                float maxY = std::max(m_boxSelect.startPos.y, m_boxSelect.endPos.y);
                
                // Only do box selection if we dragged a meaningful distance
                if (maxX - minX > 5 && maxY - minY > 5 && brushes) {
                    for (auto& brush : *brushes) {
                        if (!brush.isVisible) continue;
                        
                        // Project brush center to screen
                        Genesis::Vec3 brushCenter = brush.GetCenter();  // Position IS center
                        Genesis::Mat4 vp = viewport->GetCamera().GetViewProjectionMatrix();
                        Genesis::Vec4 clip = vp * Genesis::Vec4(brushCenter, 1.0f);
                        
                        if (clip.w > 0) {
                            Genesis::Vec2 ndc(clip.x / clip.w, clip.y / clip.w);
                            float screenX = (ndc.x * 0.5f + 0.5f) * viewport->GetWidth();
                            float screenY = (0.5f - ndc.y * 0.5f) * viewport->GetHeight();
                            
                            if (screenX >= minX && screenX <= maxX && screenY >= minY && screenY <= maxY) {
                                selection->Select(&brush, true);
                            }
                        }
                    }
                }
                
                m_boxSelect.active = false;
            }
        }
        
        // Gizmo dragging
        if (gizmo && selection->HasSelection() && !m_boxSelect.active) {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !gizmo->IsDragging()) {
                Genesis::Ray ray = viewport->ScreenToWorldRay(localX, localY);
                GizmoAxis axis = gizmo->HitTest(ray);
                if (axis != GizmoAxis::None) {
                    gizmo->BeginDrag(axis, ray);
                }
            } else if (gizmo->IsDragging()) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    Genesis::Ray ray = viewport->ScreenToWorldRay(localX, localY);
                    Genesis::Vec3 delta = gizmo->UpdateDrag(ray, grid);
                    
                    for (auto* brush : selection->GetSelectedBrushes()) {
                        brush->brush.position += delta;
                    }
                    for (auto* entity : selection->GetSelectedEntities()) {
                        entity->entity.position += delta;
                    }
                    gizmo->SetPosition(selection->GetSelectionCenter());
                } else {
                    gizmo->EndDrag();
                }
            }
        }
        
    } else if (currentTool == EditorTool::Block) {
        // Block creation tool
        if (blockTool) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                blockTool->OnMouseDown(viewport, localX, localY, grid);
            } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && blockTool->IsActive()) {
                blockTool->OnMouseDrag(viewport, localX, localY, grid);
            } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && blockTool->IsActive()) {
                EditorBrush newBrush = blockTool->OnMouseUp(viewport, localX, localY, grid);
                if (newBrush.brush.size.x > 0 && newBrush.brush.size.y > 0 && newBrush.brush.size.z > 0) {
                    static uint32_t nextId = 1;
                    newBrush.editorId = nextId++;
                    brushes->push_back(newBrush);
                    selection->Select(&brushes->back(), false);
                }
            }
        }
        
    } else if (currentTool == EditorTool::Entity) {
        // Entity placement tool
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && entities) {
            Genesis::Vec3 worldPos;
            
            if (viewport->IsOrthographic()) {
                worldPos = viewport->ScreenToWorld(localX, localY, 0.0f);
            } else {
                Genesis::Ray ray = viewport->ScreenToWorldRay(localX, localY);
                float closestT = std::numeric_limits<float>::max();
                bool hitSurface = false;
                
                // 1. Check Brushes (Closest surface)
                if (brushes) {
                    for (auto& brush : *brushes) {
                        if (!brush.isVisible) continue;
                        float t = ray.IntersectAABB(brush.GetAABB());
                        if (t > 0 && t < closestT) {
                            closestT = t;
                            hitSurface = true;
                        }
                    }
                }
                
                if (hitSurface) {
                    worldPos = ray.GetPoint(closestT);
                } else {
                    // 2. Fallback to Ground Plane (Y=0)
                    float t = ray.IntersectPlane(Genesis::Vec3(0, 1, 0), 0);
                    if (t > 0) {
                        worldPos = ray.GetPoint(t);
                    } else {
                        // 3. Fallback to constant distance (sky)
                        worldPos = ray.GetPoint(100.0f);
                    }
                }
            }
            
            worldPos = grid->Snap(worldPos);
            
            EditorEntity newEntity;
            switch (entityType) {
                case EntityPaletteType::Light:
                    newEntity = EditorEntity::CreateLight(worldPos);
                    break;
                case EntityPaletteType::PlayerStart:
                    newEntity = EditorEntity::CreatePlayerStart(worldPos);
                    break;
                case EntityPaletteType::Trigger:
                    newEntity = EditorEntity::CreateTrigger(worldPos, Genesis::Vec3(64, 64, 64));
                    break;
            }
            
            static uint32_t nextEntityId = 1;
            newEntity.editorId = nextEntityId++;
            entities->push_back(newEntity);
            
            // Select the new entity
            selection->ClearSelection();
            selection->SelectEntity(&entities->back(), false);
        }
    }
}

// ============================================================================
// Raycasting
// ============================================================================

EditorBrush* ViewportLayout::RaycastBrush(Viewport* viewport, float x, float y,
                                           std::vector<EditorBrush>* brushes) {
    if (!brushes || brushes->empty()) return nullptr;
    
    Genesis::Ray ray = viewport->ScreenToWorldRay(x, y);
    
    float closestT = std::numeric_limits<float>::max();
    EditorBrush* closestBrush = nullptr;
    
    for (auto& brush : *brushes) {
        if (!brush.isVisible) continue;
        
        Genesis::AABB aabb = brush.GetAABB();
        float t = ray.IntersectAABB(aabb);
        
        if (t > 0 && t < closestT) {
            closestT = t;
            closestBrush = &brush;
        }
    }
    
    return closestBrush;
}

EditorEntity* ViewportLayout::RaycastEntity(Viewport* viewport, float x, float y,
                                             std::vector<EditorEntity>* entities) {
    if (!entities || entities->empty()) return nullptr;
    
    Genesis::Ray ray = viewport->ScreenToWorldRay(x, y);
    
    float closestT = std::numeric_limits<float>::max();
    EditorEntity* closestEntity = nullptr;
    
    for (auto& entity : *entities) {
        if (!entity.isVisible) continue;
        
        // Use a small sphere around entity position for hit testing
        float radius = 16.0f;
        Genesis::AABB aabb(
            entity.entity.position - Genesis::Vec3(radius),
            entity.entity.position + Genesis::Vec3(radius)
        );
        
        float t = ray.IntersectAABB(aabb);
        
        if (t > 0 && t < closestT) {
            closestT = t;
            closestEntity = &entity;
        }
    }
    
    return closestEntity;
}

// ============================================================================
// Viewport Lookup
// ============================================================================

Viewport* ViewportLayout::GetViewportAt(float x, float y) {
    return m_viewports[m_activeViewport].get();
}

// ============================================================================
// Transform Helpers
// ============================================================================

void ViewportLayout::StartTransform(TransformMode mode, SelectionManager* selection, const Genesis::Vec2& startMouse) {
    if (!selection->HasSelection()) return;
    
    m_transform.mode = mode;
    m_transform.axis = TransformAxis::None;
    m_transform.startMousePos = startMouse;
    m_transform.items.clear();
    
    Genesis::Vec3 sumPos(0.0f);
    int count = 0;
    
    // Brushes
    auto& brushes = selection->GetSelectedBrushes();
    for (auto* brush : brushes) {
        m_transform.items.emplace_back((void*)brush, true, brush->brush.position, brush->brush.size, brush->brush.rotation);
        sumPos += brush->brush.position;
        count++;
    }
    
    // Entities
    auto& entities = selection->GetSelectedEntities();
    for (auto* ent : entities) {
        m_transform.items.emplace_back((void*)ent, false, ent->entity.position, Genesis::Vec3(0.0f), ent->entity.rotation);
        sumPos += ent->entity.position;
        count++;
    }
    
    if (count > 0) m_transform.center = sumPos / (float)count;
    
    // Initial params
    m_transform.startDistance = 0.0f; 
    m_transform.startAngle = 0.0f;    
}

void ViewportLayout::ApplyTransform() {
    m_transform.mode = TransformMode::None;
    m_transform.items.clear();
}

void ViewportLayout::CancelTransform() {
    if (m_transform.mode == TransformMode::None) return;
    
    // Revert
    for (const auto& item : m_transform.items) {
        if (item.isBrush) {
            auto* brush = static_cast<EditorBrush*>(item.ptr);
            brush->brush.position = item.startPos;
            brush->brush.size = item.startSize;
            brush->brush.rotation = item.startRot;
        } else {
            auto* ent = static_cast<EditorEntity*>(item.ptr);
            ent->entity.position = item.startPos;
            ent->entity.rotation = item.startRot;
        }
    }
    
    m_transform.mode = TransformMode::None;
    m_transform.items.clear();
}

void ViewportLayout::UpdateTransform(const Genesis::Vec2& currentMouse, Viewport* viewport) {
    if (m_transform.mode == TransformMode::None) return;
    
    // Calculate World Delta
    Genesis::Vec3 worldDelta(0.0f);
    float angle = 0.0f;
    float scale = 1.0f;
    
    // Calculate NDC depth of the selection center
    // This is required because ScreenToWorld expects depth in NDC range [-1, 1], not linear world distance
    Genesis::Mat4 viewProj = viewport->GetCamera().GetViewProjectionMatrix();
    Genesis::Vec4 clipPos = viewProj * Genesis::Vec4(m_transform.center, 1.0f);
    float depth = clipPos.z / clipPos.w;
    
    Genesis::Vec3 camFwd = viewport->GetCamera().GetForward();
    
    // Check for orthogonality
    if (!viewport->IsPerspective()) {
        // For ortho, we can usually just preserve the center's depth?
        // Actually, ScreenToWorld with ortho and correct NDC depth should work fine.
        // But if we want to move parallel to camera plane, we use the center's fixed depth.
    }
    
    if (m_transform.mode == TransformMode::Translate) {
        Genesis::Vec3 currWorld = viewport->ScreenToWorld(currentMouse.x, currentMouse.y, depth);
        Genesis::Vec3 startWorld = viewport->ScreenToWorld(m_transform.startMousePos.x, m_transform.startMousePos.y, depth);
        worldDelta = currWorld - startWorld;
        
        // Axis Constraint
        if (m_transform.axis == TransformAxis::X) worldDelta = Genesis::Vec3(worldDelta.x, 0, 0);
        if (m_transform.axis == TransformAxis::Y) worldDelta = Genesis::Vec3(0, worldDelta.y, 0);
        if (m_transform.axis == TransformAxis::Z) worldDelta = Genesis::Vec3(0, 0, worldDelta.z);
    }
    
    if (m_transform.mode == TransformMode::Rotate) {
        float dx = currentMouse.x - m_transform.startMousePos.x;
        angle = glm::radians(dx * 0.5f); // Sensitivity
    }
    
    if (m_transform.mode == TransformMode::Scale) {
        float dx = currentMouse.x - m_transform.startMousePos.x;
        scale = 1.0f + dx * 0.01f;
    }
    
    // Apply to items
    for (const auto& item : m_transform.items) {
        if (m_transform.mode == TransformMode::Translate) {
            Genesis::Vec3 newPos = item.startPos + worldDelta;
            if (item.isBrush) static_cast<EditorBrush*>(item.ptr)->brush.position = newPos;
            else static_cast<EditorEntity*>(item.ptr)->entity.position = newPos;
        }
        else if (m_transform.mode == TransformMode::Rotate) {
            // Rotation Axis
            Genesis::Vec3 axis(0, 1, 0); // Default Y
            if (m_transform.axis == TransformAxis::X) axis = Genesis::Vec3(1, 0, 0);
            if (m_transform.axis == TransformAxis::Y) axis = Genesis::Vec3(0, 1, 0);
            if (m_transform.axis == TransformAxis::Z) axis = Genesis::Vec3(0, 0, 1);
            else if (viewport->IsPerspective()) axis = -camFwd; // Screen space rotate
            
            // Rotate position around center
            Genesis::Vec3 relPos = item.startPos - m_transform.center;
            relPos = glm::rotate(relPos, angle, axis);
            Genesis::Vec3 newPos = m_transform.center + relPos;
            
            if (item.isBrush) {
                static_cast<EditorBrush*>(item.ptr)->brush.position = newPos;
            } else {
                static_cast<EditorEntity*>(item.ptr)->entity.position = newPos;
            }
        }
        else if (m_transform.mode == TransformMode::Scale) {
            // Scale position from center
            Genesis::Vec3 relPos = item.startPos - m_transform.center;
            
            Genesis::Vec3 scaleFactor(scale);
            if (m_transform.axis == TransformAxis::X) scaleFactor = Genesis::Vec3(scale, 1, 1);
            if (m_transform.axis == TransformAxis::Y) scaleFactor = Genesis::Vec3(1, scale, 1);
            if (m_transform.axis == TransformAxis::Z) scaleFactor = Genesis::Vec3(1, 1, scale);
            
            relPos *= scaleFactor;
            Genesis::Vec3 newPos = m_transform.center + relPos;
            
            // Scale size
            Genesis::Vec3 newSize = item.startSize * scaleFactor;
            
            if (item.isBrush) {
                auto* b = static_cast<EditorBrush*>(item.ptr);
                b->brush.position = newPos;
                b->brush.size = newSize;
            } else {
                 static_cast<EditorEntity*>(item.ptr)->entity.position = newPos;
            }
        }
    }
}
