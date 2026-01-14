#version 330 core

// ============================================================================
// Glass Vertex Shader - With Tangent Space for Normal Mapping
// Adapted from Hell2025 for OpenGL 3.3
// ============================================================================

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_Tangent;
out vec3 v_BiTangent;
out vec3 v_ViewPos;

void main()
{
    vec4 worldPos = u_Model * vec4(aPos, 1.0);
    v_WorldPos = worldPos.xyz;
    
    // Transform normal, tangent to world space
    mat3 normalMatrix = mat3(transpose(inverse(u_Model)));
    v_Normal = normalize(normalMatrix * aNormal);
    v_Tangent = normalize(normalMatrix * aTangent);
    v_BiTangent = normalize(cross(v_Normal, v_Tangent));
    
    v_TexCoord = aTexCoord;
    
    // Extract camera position from inverse view matrix
    mat4 invView = inverse(u_View);
    v_ViewPos = vec3(invView[3]);
    
    gl_Position = u_Proj * u_View * worldPos;
}
