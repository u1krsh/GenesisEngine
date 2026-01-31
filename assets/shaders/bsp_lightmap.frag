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
uniform sampler2D normalMapTexture;  // Texture unit 2 (also used for height)

// =====================================================
// Material Properties
// =====================================================
uniform vec3 u_Color;
uniform int hasDiffuseTexture;
uniform int hasLightmap;
uniform int hasNormalMap;

// =====================================================
// Normal Map Enhancement Settings
// =====================================================
uniform float u_NormalStrength;   // Normal map intensity (default: 1.0, range 0.0-2.0)
uniform float u_Roughness;        // Surface roughness for specular (default: 0.5)

// =====================================================
// Parallax Occlusion Mapping Settings
// =====================================================
uniform float u_HeightScale;      // Depth of the effect (default: 0.05)
uniform int u_EnablePOM;          // 0 = normal mapping only, 1 = parallax occlusion

// =====================================================
// Lighting Uniforms
// =====================================================
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientColor;
uniform vec3 u_ViewPos;

// Debug mode
uniform int u_DebugNormalMap;

// =====================================================
// Parallax Occlusion Mapping Function
// =====================================================
vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDirTangent) {
    // Number of depth layers - more = better quality, higher cost
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    
    // More layers when looking at steep angles
    float numLayers = mix(maxLayers, minLayers, abs(viewDirTangent.z));
    
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    
    // Amount to shift texture coordinates per layer
    vec2 P = viewDirTangent.xy / viewDirTangent.z * u_HeightScale;
    vec2 deltaTexCoords = P / numLayers;
    
    vec2 currentTexCoords = texCoords;
    
    // Sample height from normal map alpha or derive from blue channel
    // Most normal maps: blue channel represents "up" (height)
    float currentDepthMapValue = 1.0 - texture(normalMapTexture, currentTexCoords).b;
    
    // Raymarch through height field
    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - texture(normalMapTexture, currentTexCoords).b;
        currentLayerDepth += layerDepth;
    }
    
    // Parallax occlusion mapping with linear interpolation
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = (1.0 - texture(normalMapTexture, prevTexCoords).b) - currentLayerDepth + layerDepth;
    
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    
    return finalTexCoords;
}

// =====================================================
// Self-Shadowing for POM
// =====================================================
float ParallaxSoftShadow(vec2 texCoords, vec3 lightDirTangent) {
    if (lightDirTangent.z <= 0.0) return 0.0;
    
    const float minLayers = 4.0;
    const float maxLayers = 16.0;
    float numLayers = mix(maxLayers, minLayers, abs(lightDirTangent.z));
    
    float layerDepth = 1.0 / numLayers;
    vec2 deltaTexCoords = lightDirTangent.xy / lightDirTangent.z * u_HeightScale / numLayers;
    
    float currentDepth = 1.0 - texture(normalMapTexture, texCoords).b;
    float currentLayerDepth = currentDepth;
    
    float shadow = 0.0;
    float numShadowSamples = 0.0;
    
    while (currentLayerDepth > 0.0) {
        texCoords += deltaTexCoords;
        float sampledDepth = 1.0 - texture(normalMapTexture, texCoords).b;
        
        if (sampledDepth < currentLayerDepth) {
            shadow += (currentLayerDepth - sampledDepth);
            numShadowSamples += 1.0;
        }
        
        currentLayerDepth -= layerDepth;
    }
    
    if (numShadowSamples > 0.0) {
        shadow /= numShadowSamples;
    }
    
    return 1.0 - clamp(shadow * 2.0, 0.0, 1.0);
}

void main() {
    // =====================================================
    // Calculate view direction in tangent space (for POM)
    // =====================================================
    vec3 viewDirWorld = normalize(u_ViewPos - FragPos);
    vec3 viewDirTangent = normalize(transpose(TBN) * viewDirWorld);
    
    // =====================================================
    // Apply Parallax Occlusion Mapping
    // =====================================================
    vec2 texCoords = TexCoord;
    
    if (hasNormalMap != 0 && u_EnablePOM != 0) {
        texCoords = ParallaxOcclusionMapping(TexCoord, viewDirTangent);
        
        // Discard fragments outside texture bounds (for edge cases)
        if (texCoords.x < 0.0 || texCoords.x > 1.0 || texCoords.y < 0.0 || texCoords.y > 1.0) {
            // For repeating textures, wrap instead of discard
            texCoords = fract(texCoords);
        }
    }
    
    // =====================================================
    // Sample Diffuse Texture with parallax-adjusted UVs
    // =====================================================
    float alpha = 1.0;
    vec3 albedo = u_Color * VertexColor;
    
    if (hasDiffuseTexture != 0) {
        vec4 texColor = texture(diffuseTexture, texCoords);
        albedo *= texColor.rgb;
        alpha = texColor.a;
    }
    
    if (alpha < 0.01) {
        discard;
    }
    
    // =====================================================
    // Normal Mapping with parallax-adjusted UVs
    // =====================================================
    vec3 N;
    float normalStrength = max(u_NormalStrength, 0.0);  // Clamp to positive
    if (normalStrength == 0.0) normalStrength = 1.0;    // Default if not set
    
    // Estimate ambient occlusion from normal map height variance
    float ao = 1.0;
    
    if (hasNormalMap != 0) {
        vec3 normalTangent = texture(normalMapTexture, texCoords).rgb;
        normalTangent = normalTangent * 2.0 - 1.0;
        normalTangent.y = -normalTangent.y;  // DirectX format conversion
        
        // Apply normal strength - blend between flat normal (0,0,1) and sampled normal
        normalTangent.xy *= normalStrength;
        normalTangent = normalize(normalTangent);
        
        N = normalize(TBN * normalTangent);
        
        // Estimate AO from normal map - surfaces facing away from up have more occlusion
        // The more perpendicular the tangent-space normal, the more it's in a crevice
        float normalHeight = normalTangent.z;  // 1.0 = flat, <1.0 = angled
        ao = mix(0.55, 1.0, normalHeight * normalHeight);  // Subtle darkening in crevices
        
        // Debug visualization
        if (u_DebugNormalMap == 1) {
            FragColor = vec4(N * 0.5 + 0.5, 1.0);
            return;
        } else if (u_DebugNormalMap == 2) {
            FragColor = vec4(normalTangent * 0.5 + 0.5, 1.0);
            return;
        } else if (u_DebugNormalMap == 4) {
            // Debug: Show estimated AO
            FragColor = vec4(vec3(ao), 1.0);
            return;
        }
    } else {
        N = normalize(TBN[2]);
    }
    
    // =====================================================
    // Enhanced Lighting (Blinn-Phong with roughness)
    // =====================================================
    vec3 L = normalize(u_LightDir);
    vec3 V = viewDirWorld;
    vec3 H = normalize(L + V);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.01);  // Avoid division by zero
    
    if (u_DebugNormalMap == 3) {
        FragColor = vec4(vec3(NdotL), 1.0);
        return;
    }
    
    // Roughness-based specular (physically-based approximation)
    float roughness = clamp(u_Roughness, 0.04, 1.0);
    if (roughness == 0.0) roughness = 0.5;  // Default if not set
    
    float NdotH = max(dot(N, H), 0.0);
    // Convert roughness to specular power (higher roughness = lower power = broader highlight)
    float specPower = mix(256.0, 4.0, roughness * roughness);  
    float specular = pow(NdotH, specPower);
    
    // Fresnel-like rim enhancement for grazing angles
    float fresnel = pow(1.0 - NdotV, 3.0) * 0.3;
    
    // =====================================================
    // Self-Shadowing (optional, adds depth)
    // =====================================================
    float shadowFactor = 1.0;
    if (hasNormalMap != 0 && u_EnablePOM != 0 && NdotL > 0.0) {
        vec3 lightDirTangent = normalize(transpose(TBN) * L);
        shadowFactor = ParallaxSoftShadow(texCoords, lightDirTangent);
    }
    
    // =====================================================
    // Combine Lighting with improved balance
    // =====================================================
    vec3 lightmapLight = vec3(1.0);
    if (hasLightmap != 0) {
        lightmapLight = texture(lightmapTexture, LightmapCoord).rgb;
    }
    
    vec3 finalLighting;
    
    if (hasNormalMap != 0) {
        // Enhanced ambient with AO
        vec3 ambient = u_AmbientColor * lightmapLight * 0.25 * ao;
        
        // Diffuse with wrapped lighting for softer falloff
        float wrappedNdotL = (NdotL + 0.25) / 1.25;  // Soft wrap
        vec3 diffuse = u_LightColor * wrappedNdotL * shadowFactor * 0.65;
        
        // Specular with fresnel boost
        vec3 spec = u_LightColor * (specular + fresnel) * shadowFactor * (1.0 - roughness) * 0.5;
        
        // Lightmap integration - modulated by normal response
        vec3 lightmapContrib = lightmapLight * (0.35 + 0.65 * NdotL * shadowFactor) * ao;
        
        finalLighting = ambient + lightmapContrib * 0.55 + diffuse + spec;
    } else {
        finalLighting = lightmapLight;
    }
    
    // =====================================================
    // Final Output
    // =====================================================
    vec3 result = albedo * finalLighting;
    result = pow(result, vec3(1.0 / 2.2));  // Gamma correction
    result = min(result, vec3(1.0));
    
    FragColor = vec4(result, alpha);
}
