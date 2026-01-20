#version 330 core

// Inputs from vertex shader
in vec2 TexCoord;
in vec2 LightmapCoord;
in vec3 VertexColor;
in vec3 FragPos;
in mat3 TBN;

out vec4 FragColor;

// =====================================================
// Texture Samplers
// =====================================================
uniform sampler2D diffuseTexture;    // Texture unit 0
uniform sampler2D lightmapTexture;   // Texture unit 1
uniform sampler2D normalMapTexture;  // Texture unit 2 - MUST be loaded as LINEAR, not sRGB!

// =====================================================
// Material Properties
// =====================================================
uniform vec3 u_Color;
uniform int hasDiffuseTexture;
uniform int hasLightmap;
uniform int hasNormalMap;

// =====================================================
// Lighting Uniforms
// =====================================================
uniform vec3 u_LightDir;        // Direction FROM surface TO light (normalized)
uniform vec3 u_LightColor;      // Light color/intensity
uniform vec3 u_AmbientColor;    // Ambient light color
uniform vec3 u_ViewPos;         // Camera position for specular

// =====================================================
// Debug Mode (for troubleshooting)
// 0 = off, 1 = world normals, 2 = tangent-space normals, 3 = NdotL
// =====================================================
uniform int u_DebugNormalMap;

void main() {
    // =====================================================
    // Sample Diffuse (Albedo) Texture
    // =====================================================
    float alpha = 1.0;
    vec3 albedo = u_Color * VertexColor;
    
    if (hasDiffuseTexture != 0) {
        vec4 texColor = texture(diffuseTexture, TexCoord);
        albedo *= texColor.rgb;
        alpha = texColor.a;
    }
    
    // Alpha test
    if (alpha < 0.01) {
        discard;
    }
    
    // =====================================================
    // Normal Mapping
    // =====================================================
    vec3 N;
    
    if (hasNormalMap != 0) {
        // Sample normal map (stored as RGB in [0,1] range)
        vec3 normalTangent = texture(normalMapTexture, TexCoord).rgb;
        
        // Convert from [0,1] to [-1,1]
        // Normal maps encode direction: (0.5, 0.5, 1.0) = (0, 0, 1) = straight up
        normalTangent = normalTangent * 2.0 - 1.0;
        
        // Handle DirectX vs OpenGL normal map format
        // DirectX normal maps have inverted Y (green channel)
        // Most game asset normal maps use DirectX convention
        normalTangent.y = -normalTangent.y;
        
        // Transform from tangent space to world space
        // TBN matrix columns are: Tangent, Bitangent, Normal
        N = normalize(TBN * normalTangent);
        
        // Debug visualization
        if (u_DebugNormalMap == 1) {
            // World-space normal as RGB color
            FragColor = vec4(N * 0.5 + 0.5, 1.0);
            return;
        } else if (u_DebugNormalMap == 2) {
            // Tangent-space normal from texture
            FragColor = vec4(normalTangent * 0.5 + 0.5, 1.0);
            return;
        }
    } else {
        // No normal map - use geometric normal (third column of TBN)
        N = normalize(TBN[2]);
    }
    
    // =====================================================
    // Lighting Calculation (Blinn-Phong)
    // =====================================================
    vec3 L = normalize(u_LightDir);           // Light direction
    vec3 V = normalize(u_ViewPos - FragPos);  // View direction
    vec3 H = normalize(L + V);                // Half vector for Blinn-Phong
    
    // Diffuse (Lambert)
    float NdotL = max(dot(N, L), 0.0);
    
    // Debug: show NdotL as grayscale
    if (u_DebugNormalMap == 3) {
        FragColor = vec4(vec3(NdotL), 1.0);
        return;
    }
    
    // Specular (Blinn-Phong)
    float NdotH = max(dot(N, H), 0.0);
    float specularPower = 64.0;  // Shininess
    float specular = pow(NdotH, specularPower);
    
    // =====================================================
    // Combine Lighting
    // =====================================================
    vec3 lightmapLight = vec3(1.0);
    if (hasLightmap != 0) {
        lightmapLight = texture(lightmapTexture, LightmapCoord).rgb;
    }
    
    vec3 finalLighting;
    
    if (hasNormalMap != 0) {
        // With normal map: blend lightmap with dynamic lighting
        vec3 ambient = u_AmbientColor * lightmapLight * 0.3;
        vec3 diffuse = u_LightColor * NdotL * 0.7;
        vec3 spec = u_LightColor * specular * 0.4;
        
        // Add lightmap contribution modulated by NdotL for more visible bumps
        vec3 lightmapContrib = lightmapLight * (0.4 + 0.6 * NdotL);
        
        finalLighting = ambient + lightmapContrib * 0.5 + diffuse + spec;
    } else {
        // Without normal map: just use lightmap
        finalLighting = lightmapLight;
    }
    
    // =====================================================
    // Final Color
    // =====================================================
    vec3 result = albedo * finalLighting;
    
    // Gamma correction (linear to sRGB)
    result = pow(result, vec3(1.0 / 2.2));
    
    // Clamp to prevent over-bright areas
    result = min(result, vec3(1.0));
    
    FragColor = vec4(result, alpha);
}


