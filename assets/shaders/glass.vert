#version 330 core

// ============================================================================
// Glass Vertex Shader - For use with BSP geometry
// Matches BSP vertex layout: position(0), normal(1), texcoord(2), color(3), lightmapCoord(4)
// ============================================================================

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;
layout(location = 4) in vec2 aLightmapCoord;  // Not used by glass, but in BSP layout

// Outputs to fragment shader
out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_Tangent;
out vec3 v_BiTangent;
out vec3 v_ViewPos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;
uniform vec3 u_CameraPos;  // Camera world position

void main() {
    // World space position
    vec4 worldPos = u_Model * vec4(aPos, 1.0);
    v_WorldPos = worldPos.xyz;
    
    // Pass through texture coordinate
    v_TexCoord = aTexCoord;
    
    // Transform normal to world space (assuming uniform scale)
    mat3 normalMatrix = mat3(transpose(inverse(u_Model)));
    v_Normal = normalize(normalMatrix * aNormal);
    
    // Compute tangent/bitangent from normal (procedural TBN)
    // This is a fallback when no actual tangent data is available
    vec3 up = abs(v_Normal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    v_Tangent = normalize(cross(up, v_Normal));
    v_BiTangent = normalize(cross(v_Normal, v_Tangent));
    
    // View position (camera position in world space)
    // If u_CameraPos is not set, derive from view matrix
    if (length(u_CameraPos) > 0.001) {
        v_ViewPos = u_CameraPos;
    } else {
        // Derive camera position from inverse view matrix
        mat4 invView = inverse(u_View);
        v_ViewPos = vec3(invView[3]);
    }
    
    // Final clip space position
    gl_Position = u_Proj * u_View * worldPos;
}
