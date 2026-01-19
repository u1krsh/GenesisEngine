#version 330 core

// ============================================================================
// Realistic Glass Fragment Shader - BSP Engine Optimized
// ============================================================================

out vec4 FragColor;

in vec3 WorldPos;
in vec3 WorldNormal;
in vec2 TexCoord;
in vec2 LightmapCoord;
in vec3 VertexColor;
in vec3 ViewDir;

// Scene color texture (framebuffer copy - for refraction)
uniform sampler2D sceneColor;
uniform vec2 screenSize;

// Glass material properties
uniform vec3 u_GlassTint;
uniform float u_IOR;
uniform float u_Thickness;
uniform float u_FresnelPower;
uniform float u_Absorption;
uniform float u_Roughness;
uniform float u_Alpha;

// Lighting
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientColor;

// Texture
uniform sampler2D diffuseTexture;
uniform int hasDiffuseTexture;

uniform sampler2D noiseTexture;
uniform int hasNoiseTexture;

// ============================================================================
// Fresnel with base reflectivity
// ============================================================================
float FresnelSchlick(float cosTheta, float F0) {
    float fresnel = F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), u_FresnelPower);
    return max(fresnel, 0.08);  // Minimum 8% reflection
}

// ============================================================================
// Environment Reflection
// ============================================================================
vec3 SampleEnvironment(vec3 dir) {
    float t = dir.y * 0.5 + 0.5;
    vec3 horizon = vec3(0.7, 0.8, 0.9);
    vec3 sky = vec3(0.4, 0.55, 0.8);
    vec3 ground = vec3(0.3, 0.28, 0.25);
    
    if (t > 0.5) {
        return mix(horizon, sky, (t - 0.5) * 2.0);
    } else {
        return mix(ground, horizon, t * 2.0);
    }
}

// ============================================================================
// Absorption based on thickness
// ============================================================================
vec3 ApplyAbsorption(vec3 color, vec3 tint, float thickness) {
    vec3 absorbColor = vec3(1.0) - tint;
    vec3 absorption = exp(-absorbColor * thickness * u_Absorption * 2.0);
    return color * absorption;
}

void main() {
    vec3 N = normalize(WorldNormal);
    vec3 V = normalize(-ViewDir);
    
    // Handle back faces
    if (dot(N, V) < 0.0) {
        N = -N;
    }
    
    float NdotV = max(dot(N, V), 0.001);
    
    // ========================================================================
    // Fresnel
    // ========================================================================
    float F0 = pow((u_IOR - 1.0) / (u_IOR + 1.0), 2.0);
    float fresnel = FresnelSchlick(NdotV, F0);
    
    // ========================================================================
    // Screen-Space Refraction with EDGE FADE
    // ========================================================================
    vec2 safeScreenSize = max(screenSize, vec2(1.0));
    vec2 screenUV = gl_FragCoord.xy / safeScreenSize;
    
    // Calculate how close we are to screen edges (0 at edge, 1 at center)
    float edgeDistX = min(screenUV.x, 1.0 - screenUV.x) * 2.0;
    float edgeDistY = min(screenUV.y, 1.0 - screenUV.y) * 2.0;
    float edgeFade = min(edgeDistX, edgeDistY);
    edgeFade = smoothstep(0.0, 0.15, edgeFade);  // Fade in over 15% from edge
    
    // Small refraction offset based on normal, FADED at edges
    vec2 refrOffset = N.xy * u_Thickness * 0.015 * edgeFade * (1.0 - u_Roughness);
    
    // Apply noise for frosted glass
    if (hasNoiseTexture > 0 && u_Roughness > 0.01) {
        float noise = texture(noiseTexture, TexCoord * 4.0).r;
        refrOffset += vec2(noise - 0.5) * u_Roughness * 0.015 * edgeFade;
    }
    
    vec2 refrUV = screenUV + refrOffset;
    refrUV = clamp(refrUV, 0.005, 0.995);
    
    vec3 refracted = texture(sceneColor, refrUV).rgb;
    
    // Apply absorption
    refracted = ApplyAbsorption(refracted, u_GlassTint, u_Thickness);
    
    // ========================================================================
    // Reflection
    // ========================================================================
    vec3 reflDir = reflect(-V, N);
    vec3 reflected = SampleEnvironment(reflDir);
    
    // Specular
    vec3 L = normalize(-u_LightDir);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 48.0 / max(u_Roughness * 4.0 + 0.1, 0.1));
    reflected += u_LightColor * spec;
    
    // ========================================================================
    // Combine
    // ========================================================================
    vec3 color = mix(refracted, reflected, fresnel);
    
    // Tint
    color *= mix(vec3(1.0), u_GlassTint, 0.2);
    
    // Texture overlay
    if (hasDiffuseTexture > 0) {
        vec4 texColor = texture(diffuseTexture, TexCoord);
        color *= mix(vec3(1.0), texColor.rgb, texColor.a * 0.5);
    }
    
    // Rim light
    float rim = 1.0 - NdotV;
    color += u_LightColor * pow(rim, 3.0) * 0.1;
    color += u_AmbientColor * 0.04;
    
    // ========================================================================
    // Alpha
    // ========================================================================
    float alpha = u_Alpha + fresnel * 0.2 + u_Thickness * 0.15;
    alpha = clamp(alpha, 0.3, 0.88);
    
    FragColor = vec4(color, alpha);
}
