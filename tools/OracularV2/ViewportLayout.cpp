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
            
            // Handle viewport input when hovered
            if (ImGui::IsItemHovered()) {
                m_activeViewport = i;
                
                // Handle input
                HandleViewportInput(m_viewports[i].get(), i, grid, 
                                   brushes, entities, selection, gizmo, blockTool, 
                                   currentTool, entityType);
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

    // Trigger Transforms (G, R, S)
    if (currentTool == EditorTool::Select && ImGui::IsWindowFocused()) {
        if (ImGui::IsKeyPressed(ImGuiKey_G)) StartTransform(TransformMode::Translate, selection, Genesis::Vec2(localX, localY));
        if (ImGui::IsKeyPressed(ImGuiKey_R)) StartTransform(TransformMode::Rotate, selection, Genesis::Vec2(localX, localY));
        if (ImGui::IsKeyPressed(ImGuiKey_S)) StartTransform(TransformMode::Scale, selection, Genesis::Vec2(localX, localY));
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
    
    // Mouse wheel zoom (Standard)
    if (io.MouseWheel != 0) {
        viewport->Zoom(io.MouseWheel);
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
    
    // Left click handling based on tool
    if (currentTool == EditorTool::Select) {
        // Selection tool
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            EditorBrush* hitBrush = RaycastBrush(viewport, localX, localY, brushes);
            EditorEntity* hitEntity = RaycastEntity(viewport, localX, localY, entities);
            
            bool addToSelection = io.KeyCtrl || io.KeyShift;
            
            if (hitBrush) {
                if (addToSelection) {
                    selection->ToggleSelection(hitBrush);
                } else {
                    selection->Select(hitBrush, false);
                }
            } else if (hitEntity) {
                // TODO: Proper entity selection
                if (!addToSelection) {
                    selection->ClearSelection();
                }
                selection->SelectEntity(&hitEntity->entity, addToSelection);
            } else if (!addToSelection) {
                selection->ClearSelection();
            }
        }
        
        // Gizmo dragging
        if (gizmo && selection->HasSelection()) {
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
                        entity->position += delta;
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
                float t = ray.IntersectPlane(Genesis::Vec3(0, 1, 0), 0);
                if (t > 0) {
                    worldPos = ray.GetPoint(t);
                } else {
                    worldPos = ray.GetPoint(100.0f);
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
            selection->SelectEntity(&entities->back().entity, false);
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
        m_transform.items.emplace_back((void*)ent, false, ent->position, Genesis::Vec3(0.0f), ent->rotation);
        sumPos += ent->position;
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
            auto* ent = static_cast<Genesis::MapEntity*>(item.ptr);
            ent->position = item.startPos;
            ent->rotation = item.startRot;
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
    
    // Get depth of center
    Genesis::Vec3 camPos = viewport->GetCamera().GetPosition();
    Genesis::Vec3 camFwd = viewport->GetCamera().GetForward();
    float depth = glm::dot(m_transform.center - camPos, camFwd);
    
    // Check for orthogonality
    if (!viewport->IsPerspective()) {
        depth = 0.0f; 
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
            else static_cast<Genesis::MapEntity*>(item.ptr)->position = newPos;
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
                static_cast<Genesis::MapEntity*>(item.ptr)->position = newPos;
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
                 static_cast<Genesis::MapEntity*>(item.ptr)->position = newPos;
            }
        }
    }
}
