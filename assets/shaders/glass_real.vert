#version 330 core

// ============================================================================
// Realistic Glass Vertex Shader - BSP Engine
// Matches BSP vertex layout: position(0), normal(1), texcoord(2), color(3), lightmapCoord(4)
// ============================================================================

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;
layout(location = 4) in vec2 aLightmapCoord;

out vec3 WorldPos;
out vec3 WorldNormal;
out vec2 TexCoord;
out vec2 LightmapCoord;
out vec3 VertexColor;
out vec3 ViewDir;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    // World space position
    vec4 worldPosition = u_Model * vec4(aPos, 1.0);
    WorldPos = worldPosition.xyz;
    
    // Normal matrix for correct normal transformation
    mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
    WorldNormal = normalize(normalMatrix * aNormal);
    
    // Pass through texture coordinates
    TexCoord = aTexCoord;
    LightmapCoord = aLightmapCoord;
    VertexColor = aColor;
    
    // Calculate view direction (from surface to camera)
    // Extract camera position from inverse view matrix
    vec3 cameraPos = inverse(u_View)[3].xyz;
    ViewDir = WorldPos - cameraPos;  // Fragment shader will normalize
    
    gl_Position = u_Projection * u_View * worldPosition;
}
