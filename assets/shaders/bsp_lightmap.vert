#version 330 core

// Vertex attributes
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;      // UV1: Diffuse texture
layout(location = 3) in vec3 aColor;
layout(location = 4) in vec2 aLightmapCoord; // UV2: Lightmap texture
layout(location = 5) in vec3 aTangent;       // Tangent for TBN matrix

// Outputs to fragment shader
out vec2 TexCoord;
out vec2 LightmapCoord;
out vec3 VertexColor;
out vec3 FragPos;       // World-space fragment position
out mat3 TBN;           // Tangent-Bitangent-Normal matrix for normal mapping

// Uniforms
uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

void main() {
    // Transform position to world space
    vec4 worldPos = u_Model * vec4(aPos, 1.0);
    gl_Position = u_Proj * u_View * worldPos;
    
    // Pass through interpolated data
    TexCoord = aTexCoord;
    LightmapCoord = aLightmapCoord;
    VertexColor = aColor;
    FragPos = worldPos.xyz;
    
    // =====================================================
    // TBN Matrix Construction for Normal Mapping
    // =====================================================
    
    // Normal matrix: inverse-transpose of upper-left 3x3 of model matrix
    // Correctly handles non-uniform scaling
    mat3 normalMatrix = mat3(transpose(inverse(u_Model)));
    
    // Transform normal and tangent to world space
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    
    // Gram-Schmidt re-orthogonalization
    // Removes any N component from T to ensure perpendicularity
    // This is critical after interpolation and non-uniform transforms
    T = normalize(T - dot(T, N) * N);
    
    // Compute bitangent as cross product
    // This ensures a right-handed coordinate system
    // The cross product order (N × T vs T × N) determines handedness
    vec3 B = cross(N, T);
    
    // Construct TBN matrix: columns are T, B, N
    // When multiplied by tangent-space vector, gives world-space vector
    TBN = mat3(T, B, N);
}

