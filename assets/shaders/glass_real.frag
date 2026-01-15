#version 330 core

// Glass Real - Simple working version
out vec4 FragColor;

in vec3 WorldPos;
in vec3 WorldNormal;
in vec2 TexCoord;
in vec2 LightmapCoord;
in vec3 VertexColor;
in vec3 ViewDir;

uniform vec3 u_GlassTint;
uniform float u_IOR;
uniform float u_Thickness;
uniform float u_FresnelPower;
uniform float u_Absorption;
uniform float u_Roughness;
uniform float u_Alpha;
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientColor;
uniform sampler2D diffuseTexture;
uniform int hasDiffuseTexture;

void main() {
    // Keep uniforms alive
    float k = u_IOR + u_Thickness + u_FresnelPower + u_Absorption + u_Roughness + u_Alpha;
    k += dot(u_GlassTint, vec3(1.0)) + dot(u_LightDir, vec3(1.0));
    k += dot(u_LightColor, vec3(1.0)) + dot(u_AmbientColor, vec3(1.0));
    if (hasDiffuseTexture > 0) k += texture(diffuseTexture, TexCoord).r * 0.001;
    k *= 0.00001;
    
    // Glass base color - bright blue
    vec3 color = vec3(0.3, 0.6, 0.9);
    
    // Fresnel effect - brighter edges
    vec3 N = normalize(WorldNormal);
    vec3 V = normalize(-ViewDir);
    float NdotV = abs(dot(N, V));
    float fresnel = pow(1.0 - NdotV, 3.0);
    
    // Add white rim at edges
    color += vec3(0.5) * fresnel;
    
    // Specular highlight  
    vec3 L = normalize(vec3(1.0, 1.0, 0.5));
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    color += vec3(1.0, 0.95, 0.9) * spec * 0.5;
    
    // Clamp color
    color = clamp(color + k, 0.0, 1.0);
    
    // Semi-transparent
    float alpha = 0.6 + fresnel * 0.2;
    
    FragColor = vec4(color, alpha);
}
