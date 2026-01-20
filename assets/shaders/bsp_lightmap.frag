#version 330 core

in vec2 TexCoord;
in vec2 LightmapCoord;
in vec3 VertexColor;
in vec3 FragPos;
in mat3 TBN;

out vec4 FragColor;

// Textures
uniform sampler2D diffuseTexture;    // Texture unit 0
uniform sampler2D lightmapTexture;   // Texture unit 1
uniform sampler2D normalMapTexture;  // Texture unit 2

// Material
uniform vec3 u_Color;
uniform float u_Alpha;
uniform bool hasDiffuseTexture;
uniform bool hasLightmap;
uniform bool hasNormalMap;

// Lighting (for normal mapped surfaces)
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientColor;

void main() {
    // Default to fully opaque
    float alpha = 1.0;
    
    // Diffuse color from texture or base color
    vec3 diffuse = u_Color * VertexColor;
    
    if (hasDiffuseTexture) {
        vec4 texColor = texture(diffuseTexture, TexCoord);
        diffuse *= texColor.rgb;
        
        // Use texture alpha directly - transparent PNG parts become transparent
        alpha = texColor.a;
    }
    
    // Discard fully transparent pixels
    if (alpha < 0.01) {
        discard;
    }
    
    // Normal mapping
    vec3 normal = TBN[2]; // Default to vertex normal (third column of TBN is N)
    
    if (hasNormalMap) {
        // Sample normal map (stored in tangent space, range [0,1])
        vec3 normalSample = texture(normalMapTexture, TexCoord).rgb;
        
        // Convert from [0,1] to [-1,1] range
        normalSample = normalSample * 2.0 - 1.0;
        
        // Transform from tangent space to world space using TBN matrix
        normal = normalize(TBN * normalSample);
    }
    
    // Lightmap (pre-baked lighting)
    vec3 light = vec3(1.0);  // Full brightness fallback
    
    if (hasLightmap) {
        light = texture(lightmapTexture, LightmapCoord).rgb;
        
        // If we have normal map, apply additional directional shading
        if (hasNormalMap) {
            // Simple directional component based on normal map
            vec3 lightDir = normalize(u_LightDir);
            float NdotL = max(dot(normal, -lightDir), 0.0);
            
            // Blend lightmap with normal-mapped directional lighting
            // This adds surface detail while preserving baked shadows
            vec3 directional = u_LightColor * NdotL * 0.3;
            light = light + directional;
        }
    } else {
        // No lightmap - use dynamic lighting with normal mapping
        if (hasNormalMap) {
            vec3 lightDir = normalize(u_LightDir);
            float NdotL = max(dot(normal, -lightDir), 0.0);
            light = u_AmbientColor + u_LightColor * NdotL;
        }
    }
    
    // Final = diffuse * light
    vec3 result = diffuse * light;
    
    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));
    
    FragColor = vec4(result, alpha);
}
