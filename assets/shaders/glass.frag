#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 ClipSpacePos;

// Camera
uniform vec3 u_ViewPos;

// Glass material properties
uniform vec3 u_TintColor;         // Glass tint color (default: white)
uniform float u_Opacity;          // Base transparency (0.0 = invisible, 1.0 = opaque)
uniform float u_RefractStrength;  // Distortion strength (default: 0.02)
uniform float u_FresnelPower;     // Edge glow intensity (default: 3.0)
uniform float u_IOR;              // Index of refraction (1.5 for glass)

// Scene texture (rendered before glass pass)
uniform sampler2D u_SceneTexture;

// Lighting (simplified)
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform float u_AmbientStrength;

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(u_ViewPos - FragPos);
    
    // =========================================================================
    // Fresnel Effect
    // =========================================================================
    // Surfaces are more reflective at glancing angles
    float NdotV = max(dot(N, V), 0.0);
    float fresnel = pow(1.0 - NdotV, u_FresnelPower);
    fresnel = clamp(fresnel, 0.0, 1.0);
    
    // =========================================================================
    // Screen-Space Refraction
    // =========================================================================
    // Convert to normalized device coordinates (0 to 1)
    vec2 ndc = (ClipSpacePos.xy / ClipSpacePos.w) * 0.5 + 0.5;
    
    // Distort UV based on surface normal (simulate light bending)
    vec2 distortion = N.xy * u_RefractStrength;
    vec2 refractUV = ndc + distortion;
    
    // Clamp to prevent sampling outside texture
    refractUV = clamp(refractUV, 0.002, 0.998);
    
    // Sample the scene behind the glass
    vec3 refractColor = texture(u_SceneTexture, refractUV).rgb;
    
    // =========================================================================
    // Simple Specular Highlight
    // =========================================================================
    vec3 L = normalize(-u_LightDir);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 64.0);  // High shininess for glass
    vec3 specular = spec * u_LightColor * 0.5;
    
    // =========================================================================
    // Combine Effects
    // =========================================================================
    // Base glass color: blend refracted scene with tint
    vec3 glassColor = mix(refractColor, u_TintColor * refractColor, 0.3);
    
    // Add specular highlights
    glassColor += specular;
    
    // Add subtle ambient
    glassColor += u_TintColor * u_AmbientStrength * 0.1;
    
    // =========================================================================
    // Alpha Calculation
    // =========================================================================
    // Base opacity + fresnel makes edges more visible
    float alpha = u_Opacity + fresnel * (1.0 - u_Opacity) * 0.5;
    
    // Add a bit more opacity where specular is strong
    alpha = clamp(alpha + spec * 0.3, 0.0, 1.0);
    
    FragColor = vec4(glassColor, alpha);
}
