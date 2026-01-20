#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;      // UV1: Diffuse texture
layout(location = 3) in vec3 aColor;
layout(location = 4) in vec2 aLightmapCoord; // UV2: Lightmap texture
layout(location = 5) in vec3 aTangent;       // Tangent for TBN matrix

out vec2 TexCoord;
out vec2 LightmapCoord;
out vec3 VertexColor;
out vec3 FragPos;       // World-space fragment position
out mat3 TBN;           // Tangent-Bitangent-Normal matrix for normal mapping

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

void main() {
    // Transform position
    vec4 worldPos = u_Model * vec4(aPos, 1.0);
    gl_Position = u_Proj * u_View * worldPos;
    
    // Pass through texture coordinates
    TexCoord = aTexCoord;
    LightmapCoord = aLightmapCoord;
    VertexColor = aColor;
    FragPos = worldPos.xyz;
    
    // Build TBN matrix for normal mapping
    // Transform normal and tangent to world space
    mat3 normalMatrix = mat3(transpose(inverse(u_Model)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    
    // Re-orthogonalize T with respect to N (Gram-Schmidt)
    T = normalize(T - dot(T, N) * N);
    
    // Calculate bitangent
    vec3 B = cross(N, T);
    
    // TBN matrix transforms from tangent space to world space
    TBN = mat3(T, B, N);
}
