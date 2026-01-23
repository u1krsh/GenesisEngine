// ============================================================================
// OracularV2 Viewport Implementation For the Commit
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

// Note: STB_IMAGE_IMPLEMENTATION is in engine's Texture2D.cpp
#include "stb_image.h"

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

// Billboard Sprite Shader
static const char* spriteVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 aOffset;  // Corner offset (-1 to 1)
layout(location = 1) in vec2 aTexCoord;// UV

uniform mat4 uViewProj;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform vec3 uCenter;    // Center world position
uniform float uSize;

out vec2 vTexCoord;

void main() {
    vec3 right = uCameraRight * aOffset.x * uSize * 0.5;
    vec3 up = uCameraUp * aOffset.y * uSize * 0.5;
    
    vec3 pos = uCenter + right + up;
    
    gl_Position = uViewProj * vec4(pos, 1.0);
    vTexCoord = aTexCoord;
}
)";

static const char* spriteFragmentShader = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uTint;

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);
    if (texColor.a < 0.1) discard;
    FragColor = texColor * uTint;
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
    
    // ========================================================================
    // Create Sprite Shader
    // ========================================================================
    {
        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &spriteVertexShader, nullptr);
        glCompileShader(vs);
        
        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &spriteFragmentShader, nullptr);
        glCompileShader(fs);
        
        m_spriteShader = glCreateProgram();
        glAttachShader(m_spriteShader, vs);
        glAttachShader(m_spriteShader, fs);
        glLinkProgram(m_spriteShader);
        
        glDeleteShader(vs);
        glDeleteShader(fs);
    }
    
    // ========================================================================
    // Create Sprite Quad VAO/VBO
    // ========================================================================
    glGenVertexArrays(1, &m_spriteVAO);
    glGenBuffers(1, &m_spriteVBO);
    glBindVertexArray(m_spriteVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVBO);
    
    // Quad data: OffsetX, OffsetY, U, V
    float quadVertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, // BL
         1.0f, -1.0f, 1.0f, 1.0f, // BR
         1.0f,  1.0f, 1.0f, 0.0f, // TR
         
        -1.0f, -1.0f, 0.0f, 1.0f, // BL
         1.0f,  1.0f, 1.0f, 0.0f, // TR
        -1.0f,  1.0f, 0.0f, 0.0f  // TL
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Attribs
    // 0: Offset (vec2) - using layout 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 1: TexCoord (vec2) - using layout 1
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    // ========================================================================
    // Generate Procedural Bulb Texture (Fallback)
    // ========================================================================
    glGenTextures(1, &m_spriteTexture);
    glBindTexture(GL_TEXTURE_2D, m_spriteTexture);
    
    int size = 64;
    std::vector<unsigned char> pixels(size * size * 4);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float dx = (x - size/2.0f) / (size/2.0f);
            float dy = (y - size/2.0f) / (size/2.0f);
            float dist = sqrt(dx*dx + dy*dy);
            
            unsigned char r = 255, g = 255, b = 200, a = 0;
            
            // Bulb body (circle)
            if (dist < 0.6f) {
                a = 255;
                // Highlight
                if (dx > -0.2f && dx < 0.2f && dy > -0.2f && dy < 0.2f) {
                    r = 255; g = 255; b = 255; 
                }
            }
            // Glow/Rays
            else if (dist < 0.9f) {
                // Determine angle for rays
                float angle = atan2(dy, dx);
                if (sin(angle * 8) > 0.5f) {
                    a = (unsigned char)(255 * (1.0f - dist) / 0.3f);
                }
            }
            // Socket
            if (dy > 0.5f && dx > -0.3f && dx < 0.3f) {
                r = 100; g = 100; b = 100; a = 255;
            }
            
            int idx = (y * size + x) * 4;
            pixels[idx+0] = r;
            pixels[idx+1] = g;
            pixels[idx+2] = b;
            pixels[idx+3] = a;
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // ========================================================================
    // Attempt to load real bulb.png
    // ========================================================================
#ifdef ASSETS_DIR
    std::string bulbPath = std::string(ASSETS_DIR) + "/bulb.png";
    unsigned int loadedTex = LoadTexture(bulbPath);
    if (loadedTex != 0 && loadedTex != m_spriteTexture) {
        // Success! Replace procedural texture
        glDeleteTextures(1, &m_spriteTexture);
        m_spriteTexture = loadedTex;
    }
#endif
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
        
        // Use Min/Max accessors - position is CENTER, not corner
        Genesis::Vec3 min = brush.Min();
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
                    color = Genesis::Vec4(1.0f, 1.0f, 1.0f, 1.0f);  // White tint for sprite
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
        
        if (entity.visualType == EditorEntityType::Light) {
            // Draw sprite
            float spriteSize = 24.0f; // Fixed screen size? No, world size.
            // Adjust size to be reasonable in world units.
            // 24.0f world units is HUGE. Units are usually meters or inches. Grid is 16-64 units.
            // Let's us 16 units.
            DrawBillboardSprite(pos, 16.0f, m_spriteTexture, entity.isSelected ? Genesis::Vec4(1.0f, 0.5f, 0.0f, 1.0f) : Genesis::Vec4(1,1,1,1));
            
            // Draw rang indicator
            if (entity.isSelected || entity.visualType == EditorEntityType::Light) { // Always show for lights? maybe too cluttered.
                 float radius = entity.GetLightRadius();
                 // Draw circle
                 int segments = 24;
                 for (int i = 0; i < segments; i++) {
                     float a1 = (float)i / segments * 6.28318f;
                     float a2 = (float)(i + 1) / segments * 6.28318f;
                     Genesis::Vec3 p1 = pos + Genesis::Vec3(cos(a1) * radius, 0, sin(a1) * radius);
                     Genesis::Vec3 p2 = pos + Genesis::Vec3(cos(a2) * radius, 0, sin(a2) * radius);
                     DrawLine(p1, p2, Genesis::Vec4(1.0f, 0.9f, 0.3f, 0.5f));
                 }
            }
        } else {
            // Draw a diamond shape for other entities
            DrawLine(pos + Genesis::Vec3(size, 0, 0), pos + Genesis::Vec3(0, size, 0), color);
            DrawLine(pos + Genesis::Vec3(0, size, 0), pos + Genesis::Vec3(-size, 0, 0), color);
            DrawLine(pos + Genesis::Vec3(-size, 0, 0), pos + Genesis::Vec3(0, -size, 0), color);
            DrawLine(pos + Genesis::Vec3(0, -size, 0), pos + Genesis::Vec3(size, 0, 0), color);
            
            DrawLine(pos + Genesis::Vec3(0, 0, size), pos + Genesis::Vec3(0, size, 0), color);
            DrawLine(pos + Genesis::Vec3(0, size, 0), pos + Genesis::Vec3(0, 0, -size), color);
            DrawLine(pos + Genesis::Vec3(0, 0, -size), pos + Genesis::Vec3(0, -size, 0), color);
            DrawLine(pos + Genesis::Vec3(0, -size, 0), pos + Genesis::Vec3(0, 0, size), color);
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
        m_orbitDistance = std::clamp(m_orbitDistance, 5.0f, 5000.0f);
        SetupCamera();
    }
}

void Viewport::ZoomAtPoint(float delta, float screenX, float screenY) {
    if (IsOrthographic()) {
        // Get world position under cursor BEFORE zoom
        Genesis::Vec3 worldPosBefore = ScreenToWorld(screenX, screenY, 0.0f);
        
        // Apply zoom
        float oldZoom = m_zoom;
        m_zoom *= (1.0f + delta * 0.1f);
        m_zoom = std::clamp(m_zoom, 0.01f, 100.0f);
        
        // Get world position under cursor AFTER zoom (with old pan)
        SetupCamera();
        Genesis::Vec3 worldPosAfter = ScreenToWorld(screenX, screenY, 0.0f);
        
        // Adjust pan to keep cursor over same world point
        Genesis::Vec3 correction = worldPosBefore - worldPosAfter;
        
        switch (m_type) {
            case ViewportType::TopXZ:
                m_panOffset.x += correction.x;
                m_panOffset.y += correction.z;
                break;
            case ViewportType::FrontXY:
                m_panOffset.x += correction.x;
                m_panOffset.y += correction.y;
                break;
            case ViewportType::SideYZ:
                m_panOffset.x += correction.z;
                m_panOffset.y += correction.y;
                break;
            default:
                break;
        }
        SetupCamera();
    } else {
        // 3D perspective: Zoom towards cursor by moving orbit target
        Genesis::Ray ray = ScreenToWorldRay(screenX, screenY);
        
        float zoomAmount = delta * 20.0f;
        
        // Move orbit target towards the ray direction
        Genesis::Vec3 towardsCursor = ray.direction * zoomAmount * 0.3f;
        m_orbitTarget += towardsCursor;
        
        // Also decrease orbit distance
        m_orbitDistance -= zoomAmount;
        m_orbitDistance = std::clamp(m_orbitDistance, 5.0f, 5000.0f);
        
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

void Viewport::DrawBillboardSprite(const Genesis::Vec3& pos, float size, unsigned int texture, const Genesis::Vec4& tint) {
    if (!m_spriteShader) return;
    
    glUseProgram(m_spriteShader);
    
    Genesis::Mat4 view = m_camera.GetViewMatrix();
    Genesis::Mat4 viewProj = m_camera.GetViewProjectionMatrix();
    glUniformMatrix4fv(glGetUniformLocation(m_spriteShader, "uViewProj"), 1, GL_FALSE, &viewProj[0][0]);
    
    // Extract camera vectors from View Matrix
    // View Matrix = [ RightX  RightY  RightZ  -dot(R,P) ]
    //               [ UpX     UpY     UpZ     -dot(U,P) ]
    //               [ -FwdX   -FwdY   -FwdZ   dot(F,P)  ]
    //               [ 0       0       0       1         ]
    // GLM uses column-major storage, so view[col][row].
    // Right = (view[0][0], view[1][0], view[2][0])
    // Up    = (view[0][1], view[1][1], view[2][1])
    
    // Actually, simply:
    Genesis::Vec3 camRight = Genesis::Vec3(view[0][0], view[1][0], view[2][0]);
    Genesis::Vec3 camUp    = Genesis::Vec3(view[0][1], view[1][1], view[2][1]);
    
    // Just to be sure, allow overrides if Perspective check needed, but billboards usually always face camera plane.
    
    glUniform3fv(glGetUniformLocation(m_spriteShader, "uCameraRight"), 1, &camRight[0]);
    glUniform3fv(glGetUniformLocation(m_spriteShader, "uCameraUp"), 1, &camUp[0]);
    glUniform3fv(glGetUniformLocation(m_spriteShader, "uCenter"), 1, &pos[0]);
    glUniform1f(glGetUniformLocation(m_spriteShader, "uSize"), size);
    glUniform4fv(glGetUniformLocation(m_spriteShader, "uTint"), 1, &tint[0]);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture ? texture : m_spriteTexture);
    glUniform1i(glGetUniformLocation(m_spriteShader, "uTexture"), 0);
    
    glBindVertexArray(m_spriteVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

unsigned int Viewport::LoadTexture(const std::string& path) {
    // Generate texture ID
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    // Load image data
    int width, height, nrChannels;
    // stbi_set_flip_vertically_on_load(true); // Maybe? Sprites usually are upright.
    
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = GL_RGBA;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // Parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
        return textureID;
    } else {
        // Fallback to procedural if load fails
        if (m_spriteTexture != 0) return m_spriteTexture;
        return 0; 
    }
}

