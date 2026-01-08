// ============================================================================
// OracularV2 Viewport Implementation
// ============================================================================

#include "Viewport.h"
#include "Grid.h"
#include "Gizmo.h"
#include "SelectionManager.h"
#include "EditorBrush.h"
#include "EditorEntity.h"
#include "map/Map.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

// Simple line shader (embedded)
static const char* lineVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 uViewProj;
out vec4 vColor;
void main() {
    gl_Position = uViewProj * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

static const char* lineFragmentShader = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
}
)";

// ============================================================================
// Constructor/Destructor
// ============================================================================

Viewport::Viewport(ViewportType type) 
    : m_type(type) {
    SetupCamera();
}

Viewport::~Viewport() {
    DestroyFramebuffer();
    if (m_lineVAO) glDeleteVertexArrays(1, &m_lineVAO);
    if (m_lineVBO) glDeleteBuffers(1, &m_lineVBO);
    if (m_lineShader) glDeleteProgram(m_lineShader);
}

// ============================================================================
// Initialization
// ============================================================================

void Viewport::Initialize(int width, int height) {
    m_width = width;
    m_height = height;
    CreateFramebuffer();
    SetupCamera();
    
    // Create line shader
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &lineVertexShader, nullptr);
    glCompileShader(vs);
    
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &lineFragmentShader, nullptr);
    glCompileShader(fs);
    
    m_lineShader = glCreateProgram();
    glAttachShader(m_lineShader, vs);
    glAttachShader(m_lineShader, fs);
    glLinkProgram(m_lineShader);
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    // Create line VAO/VBO
    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);
    
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferData(GL_ARRAY_BUFFER, 1024 * 7 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    
    // Position (3 floats) + Color (4 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void Viewport::Resize(int width, int height) {
    if (width == m_width && height == m_height) return;
    
    m_width = std::max(1, width);
    m_height = std::max(1, height);
    
    DestroyFramebuffer();
    CreateFramebuffer();
    SetupCamera();
}

void Viewport::SetupCamera() {
    float aspect = 1.0f;
    if (m_height > 0) {
        aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
    }
    
    if (IsPerspective()) {
        // 3D perspective camera
        m_camera.SetPerspective(60.0f, aspect, 0.1f, 10000.0f);
        
        // Calculate position from orbit
        float yawRad = glm::radians(m_orbitYaw);
        float pitchRad = glm::radians(m_orbitPitch);
        
        Genesis::Vec3 offset;
        offset.x = m_orbitDistance * cos(pitchRad) * cos(yawRad);
        offset.y = m_orbitDistance * sin(pitchRad);
        offset.z = m_orbitDistance * cos(pitchRad) * sin(yawRad);
        
        m_camera.SetPosition(m_orbitTarget + offset);
        m_camera.LookAt(m_orbitTarget);
    } else {
        // Orthographic camera for 2D views
        float size = 200.0f / m_zoom;
        m_camera.SetOrthographic(-size * aspect, size * aspect, -size, size, -10000.0f, 10000.0f);
        
        // Set camera position based on view type
        switch (m_type) {
            case ViewportType::TopXZ:
                m_camera.SetPosition(Genesis::Vec3(m_panOffset.x, 500.0f, m_panOffset.y));
                m_camera.LookAt(Genesis::Vec3(m_panOffset.x, 0.0f, m_panOffset.y), Genesis::Vec3(0.0f, 0.0f, -1.0f)); // Up is -Z
                break;
            case ViewportType::FrontXY:
                m_camera.SetPosition(Genesis::Vec3(m_panOffset.x, m_panOffset.y, 500.0f));
                m_camera.LookAt(Genesis::Vec3(m_panOffset.x, m_panOffset.y, 0.0f));
                break;
            case ViewportType::SideYZ:
                m_camera.SetPosition(Genesis::Vec3(500.0f, m_panOffset.y, m_panOffset.x));
                m_camera.LookAt(Genesis::Vec3(0.0f, m_panOffset.y, m_panOffset.x));
                break;
            default:
                break;
        }
    }
}

// ============================================================================
// Framebuffer Management
// ============================================================================

void Viewport::CreateFramebuffer() {
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
    
    glGenRenderbuffers(1, &m_depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Viewport::DestroyFramebuffer() {
    if (m_framebuffer) {
        glDeleteFramebuffers(1, &m_framebuffer);
        m_framebuffer = 0;
    }
    if (m_colorTexture) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_depthRenderbuffer) {
        glDeleteRenderbuffers(1, &m_depthRenderbuffer);
        m_depthRenderbuffer = 0;
    }
}

// ============================================================================
// Rendering
// ============================================================================

void Viewport::BeginRender() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, m_width, m_height);
    
    if (IsPerspective()) {
        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
    } else {
        glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Viewport::EndRender() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Viewport::RenderGrid(Grid* grid) {
    if (!grid || !grid->IsVisible() || !m_lineShader) return;
    
    glUseProgram(m_lineShader);
    Genesis::Mat4 viewProj = m_camera.GetViewProjectionMatrix();
    glUniformMatrix4fv(glGetUniformLocation(m_lineShader, "uViewProj"), 1, GL_FALSE, &viewProj[0][0]);
    
    // Draw grid lines
    float gridSize = grid->GetSnapSize();
    int numLines = 32;
    float extent = numLines * gridSize;
    
    Genesis::Vec4 gridColor = grid->GetGridColor();
    Genesis::Vec4 axisColor = grid->GetAxisColor();
    
    std::vector<float> vertices;
    
    // Grid lines based on viewport type
    if (m_type == ViewportType::TopXZ || IsPerspective()) {
        // XZ grid (Y = 0)
        for (int i = -numLines; i <= numLines; i++) {
            float pos = i * gridSize;
            Genesis::Vec4 color = (i == 0) ? axisColor : gridColor;
            
            // X line
            vertices.insert(vertices.end(), {-extent, 0, pos, color.r, color.g, color.b, color.a});
            vertices.insert(vertices.end(), {extent, 0, pos, color.r, color.g, color.b, color.a});
            
            // Z line
            vertices.insert(vertices.end(), {pos, 0, -extent, color.r, color.g, color.b, color.a});
            vertices.insert(vertices.end(), {pos, 0, extent, color.r, color.g, color.b, color.a});
        }
    } else if (m_type == ViewportType::FrontXY) {
        // XY grid (Z = 0)
        for (int i = -numLines; i <= numLines; i++) {
            float pos = i * gridSize;
            Genesis::Vec4 color = (i == 0) ? axisColor : gridColor;
            
            vertices.insert(vertices.end(), {-extent, pos, 0, color.r, color.g, color.b, color.a});
            vertices.insert(vertices.end(), {extent, pos, 0, color.r, color.g, color.b, color.a});
            
            vertices.insert(vertices.end(), {pos, -extent, 0, color.r, color.g, color.b, color.a});
            vertices.insert(vertices.end(), {pos, extent, 0, color.r, color.g, color.b, color.a});
        }
    } else if (m_type == ViewportType::SideYZ) {
        // YZ grid (X = 0)
        for (int i = -numLines; i <= numLines; i++) {
            float pos = i * gridSize;
            Genesis::Vec4 color = (i == 0) ? axisColor : gridColor;
            
            vertices.insert(vertices.end(), {0, -extent, pos, color.r, color.g, color.b, color.a});
            vertices.insert(vertices.end(), {0, extent, pos, color.r, color.g, color.b, color.a});
            
            vertices.insert(vertices.end(), {0, pos, -extent, color.r, color.g, color.b, color.a});
            vertices.insert(vertices.end(), {0, pos, extent, color.r, color.g, color.b, color.a});
        }
    }
    
    if (!vertices.empty()) {
        glBindVertexArray(m_lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        glDrawArrays(GL_LINES, 0, vertices.size() / 7);
        glBindVertexArray(0);
    }
}

void Viewport::RenderBrushes(std::vector<EditorBrush>* brushes, SelectionManager* selection) {
    if (!brushes || !m_lineShader) return;
    
    glUseProgram(m_lineShader);
    Genesis::Mat4 viewProj = m_camera.GetViewProjectionMatrix();
    glUniformMatrix4fv(glGetUniformLocation(m_lineShader, "uViewProj"), 1, GL_FALSE, &viewProj[0][0]);
    
    for (auto& brush : *brushes) {
        if (!brush.isVisible) continue;
        
        Genesis::Vec3 min = brush.brush.position;
        Genesis::Vec3 max = brush.Max();
        
        // Color based on selection state
        Genesis::Vec4 color;
        if (brush.isSelected) {
            color = Genesis::Vec4(1.0f, 0.5f, 0.0f, 1.0f);  // Orange for selected
        } else if (brush.isHovered) {
            color = Genesis::Vec4(0.7f, 0.7f, 0.2f, 1.0f);  // Yellow for hovered
        } else {
            color = Genesis::Vec4(0.5f, 0.5f, 0.6f, 1.0f);  // Gray for normal
        }
        
        DrawBox(min, max, color, false);
    }
}

void Viewport::RenderEntities(std::vector<EditorEntity>* entities, SelectionManager* selection) {
    if (!entities || !m_lineShader) return;
    
    glUseProgram(m_lineShader);
    Genesis::Mat4 viewProj = m_camera.GetViewProjectionMatrix();
    glUniformMatrix4fv(glGetUniformLocation(m_lineShader, "uViewProj"), 1, GL_FALSE, &viewProj[0][0]);
    
    for (auto& entity : *entities) {
        if (!entity.isVisible) continue;
        
        Genesis::Vec3 pos = entity.entity.position;
        float size = 8.0f;
        
        // Color based on selection and type
        Genesis::Vec4 color;
        if (entity.isSelected) {
            color = Genesis::Vec4(1.0f, 0.5f, 0.0f, 1.0f);  // Orange for selected
        } else {
            switch (entity.visualType) {
                case EditorEntityType::Light:
                    color = Genesis::Vec4(1.0f, 0.9f, 0.3f, 1.0f);  // Yellow
                    break;
                case EditorEntityType::PlayerStart:
                    color = Genesis::Vec4(0.2f, 1.0f, 0.2f, 1.0f);  // Green
                    break;
                case EditorEntityType::Trigger:
                    color = Genesis::Vec4(0.8f, 0.2f, 0.8f, 1.0f);  // Magenta
                    break;
                default:
                    color = Genesis::Vec4(0.5f, 0.5f, 1.0f, 1.0f);  // Blue
                    break;
            }
        }
        
        // Draw a diamond shape for entities
        DrawLine(pos + Genesis::Vec3(size, 0, 0), pos + Genesis::Vec3(0, size, 0), color);
        DrawLine(pos + Genesis::Vec3(0, size, 0), pos + Genesis::Vec3(-size, 0, 0), color);
        DrawLine(pos + Genesis::Vec3(-size, 0, 0), pos + Genesis::Vec3(0, -size, 0), color);
        DrawLine(pos + Genesis::Vec3(0, -size, 0), pos + Genesis::Vec3(size, 0, 0), color);
        
        DrawLine(pos + Genesis::Vec3(0, 0, size), pos + Genesis::Vec3(0, size, 0), color);
        DrawLine(pos + Genesis::Vec3(0, size, 0), pos + Genesis::Vec3(0, 0, -size), color);
        DrawLine(pos + Genesis::Vec3(0, 0, -size), pos + Genesis::Vec3(0, -size, 0), color);
        DrawLine(pos + Genesis::Vec3(0, -size, 0), pos + Genesis::Vec3(0, 0, size), color);
        
        // For lights, draw a radius indicator
        if (entity.visualType == EditorEntityType::Light) {
            float radius = entity.GetLightRadius() / 10.0f;  // Scale down for visibility
            radius = std::min(radius, 50.0f);
            
            // Draw a circle in XZ plane
            int segments = 16;
            for (int i = 0; i < segments; i++) {
                float a1 = (float)i / segments * 6.28318f;
                float a2 = (float)(i + 1) / segments * 6.28318f;
                Genesis::Vec3 p1 = pos + Genesis::Vec3(cos(a1) * radius, 0, sin(a1) * radius);
                Genesis::Vec3 p2 = pos + Genesis::Vec3(cos(a2) * radius, 0, sin(a2) * radius);
                DrawLine(p1, p2, Genesis::Vec4(color.r, color.g, color.b, 0.4f));
            }
        }
    }
}

void Viewport::RenderBrushPreview(const EditorBrush& preview, Grid* grid) {
    if (!m_lineShader) return;
    
    glUseProgram(m_lineShader);
    Genesis::Mat4 viewProj = m_camera.GetViewProjectionMatrix();
    glUniformMatrix4fv(glGetUniformLocation(m_lineShader, "uViewProj"), 1, GL_FALSE, &viewProj[0][0]);
    
    Genesis::Vec3 min = preview.brush.position;
    Genesis::Vec3 max = preview.Max();
    
    // Green dashed preview
    Genesis::Vec4 color(0.2f, 0.8f, 0.2f, 0.8f);
    DrawBox(min, max, color, false);
}

void Viewport::RenderGizmo(Gizmo* gizmo) {
    if (!gizmo || !m_lineShader) return;
    
    glUseProgram(m_lineShader);
    Genesis::Mat4 viewProj = m_camera.GetViewProjectionMatrix();
    glUniformMatrix4fv(glGetUniformLocation(m_lineShader, "uViewProj"), 1, GL_FALSE, &viewProj[0][0]);
    
    Genesis::Vec3 pos = gizmo->GetPosition();
    float size = gizmo->GetSize();
    
    // Disable depth test for gizmo
    glDisable(GL_DEPTH_TEST);
    
    DrawAxisGizmo(pos, size);
    
    glEnable(GL_DEPTH_TEST);
}

// ============================================================================
// Drawing Helpers
// ============================================================================

void Viewport::DrawLine(const Genesis::Vec3& start, const Genesis::Vec3& end, 
                        const Genesis::Vec4& color) {
    float vertices[] = {
        start.x, start.y, start.z, color.r, color.g, color.b, color.a,
        end.x, end.y, end.z, color.r, color.g, color.b, color.a
    };
    
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

void Viewport::DrawBox(const Genesis::Vec3& min, const Genesis::Vec3& max,
                       const Genesis::Vec4& color, bool filled) {
    // 12 edges of a box
    Genesis::Vec3 corners[8] = {
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
        {max.x, max.y, min.z},
        {min.x, max.y, min.z},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {max.x, max.y, max.z},
        {min.x, max.y, max.z}
    };
    
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},  // Front
        {4,5}, {5,6}, {6,7}, {7,4},  // Back
        {0,4}, {1,5}, {2,6}, {3,7}   // Connecting
    };
    
    std::vector<float> vertices;
    for (int i = 0; i < 12; i++) {
        Genesis::Vec3& a = corners[edges[i][0]];
        Genesis::Vec3& b = corners[edges[i][1]];
        vertices.insert(vertices.end(), {a.x, a.y, a.z, color.r, color.g, color.b, color.a});
        vertices.insert(vertices.end(), {b.x, b.y, b.z, color.r, color.g, color.b, color.a});
    }
    
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glDrawArrays(GL_LINES, 0, 24);
    glBindVertexArray(0);
}

void Viewport::DrawAxisGizmo(const Genesis::Vec3& pos, float size) {
    // X axis - Red
    DrawLine(pos, pos + Genesis::Vec3(size, 0, 0), Genesis::Vec4(1, 0, 0, 1));
    // Y axis - Green  
    DrawLine(pos, pos + Genesis::Vec3(0, size, 0), Genesis::Vec4(0, 1, 0, 1));
    // Z axis - Blue
    DrawLine(pos, pos + Genesis::Vec3(0, 0, size), Genesis::Vec4(0, 0, 1, 1));
}

// ============================================================================
// Camera Control
// ============================================================================

void Viewport::Pan(float dx, float dy) {
    if (IsOrthographic()) {
        float scale = 200.0f / m_zoom / m_height;
        m_panOffset.x -= dx * scale;
        m_panOffset.y += dy * scale;
        SetupCamera();
    } else {
        // Pan orbit target
        Genesis::Vec3 right = m_camera.GetForward();
        right = glm::cross(right, Genesis::Vec3(0, 1, 0));
        right = glm::normalize(right);
        
        m_orbitTarget -= right * dx * 0.5f;
        m_orbitTarget.y += dy * 0.5f;
        SetupCamera();
    }
}

void Viewport::Zoom(float delta) {
    if (IsOrthographic()) {
        m_zoom *= (1.0f + delta * 0.1f);
        m_zoom = std::clamp(m_zoom, 0.01f, 100.0f);
        SetupCamera();
    } else {
        m_orbitDistance -= delta * 20.0f;
        m_orbitDistance = std::max(10.0f, m_orbitDistance);
        SetupCamera();
    }
}

void Viewport::Orbit(float dx, float dy) {
    if (!IsPerspective()) return;
    
    m_orbitYaw += dx * 0.3f;
    m_orbitPitch += dy * 0.3f;
    m_orbitPitch = std::clamp(m_orbitPitch, -89.0f, 89.0f);
    
    SetupCamera();
}

void Viewport::FocusOn(const Genesis::Vec3& position) {
    if (IsPerspective()) {
        m_orbitTarget = position;
    } else {
        switch (m_type) {
            case ViewportType::TopXZ:
                m_panOffset = Genesis::Vec2(position.x, position.z);
                break;
            case ViewportType::FrontXY:
                m_panOffset = Genesis::Vec2(position.x, position.y);
                break;
            case ViewportType::SideYZ:
                m_panOffset = Genesis::Vec2(position.z, position.y);
                break;
            default:
                break;
        }
    }
    SetupCamera();
}

// ============================================================================
// Mouse Interaction
// ============================================================================

Genesis::Ray Viewport::ScreenToWorldRay(float screenX, float screenY) const {
    float ndcX = (2.0f * screenX / m_width) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY / m_height);
    
    Genesis::Mat4 invViewProj = glm::inverse(m_camera.GetViewProjectionMatrix());
    
    Genesis::Vec4 nearPoint = invViewProj * Genesis::Vec4(ndcX, ndcY, -1.0f, 1.0f);
    nearPoint /= nearPoint.w;
    Genesis::Vec4 farPoint = invViewProj * Genesis::Vec4(ndcX, ndcY, 1.0f, 1.0f);
    farPoint /= farPoint.w;
    
    Genesis::Vec3 rayOrigin = Genesis::Vec3(nearPoint);
    Genesis::Vec3 rayDir = glm::normalize(Genesis::Vec3(farPoint) - rayOrigin);
    
    return Genesis::Ray{rayOrigin, rayDir};
}

Genesis::Vec3 Viewport::ScreenToWorld(float screenX, float screenY, float depth) const {
    float ndcX = (2.0f * screenX / m_width) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY / m_height);
    
    Genesis::Mat4 invViewProj = glm::inverse(m_camera.GetViewProjectionMatrix());
    
    Genesis::Vec4 worldPos = invViewProj * Genesis::Vec4(ndcX, ndcY, depth, 1.0f);
    worldPos /= worldPos.w;
    
    return Genesis::Vec3(worldPos);
}
