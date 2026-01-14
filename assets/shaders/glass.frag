#version 330 core

// ============================================================================
// Glass Fragment Shader - PBR-lite with Fresnel and Normal Mapping
// Adapted from Hell2025 for OpenGL 3.3
// ============================================================================

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_Tangent;
in vec3 v_BiTangent;
in vec3 v_ViewPos;

out vec4 FragColor;

// Textures
uniform sampler2D u_BaseTexture;        // Base color + alpha
uniform sampler2D u_NormalTexture;      // Normal map
uniform sampler2D u_MaskTexture;        // Opacity mask (optional)
uniform bool u_HasNormalMap;
uniform bool u_HasMaskTexture;
uniform bool u_FlipNormalMapY;

// Material properties
uniform vec3 u_Color;
uniform float u_Transparency;           // 0.0 = opaque, 1.0 = fully transparent
uniform float u_FresnelPower;           // Higher = more edge reflection
uniform float u_Roughness;
uniform float u_RefractiveIndex;        // Glass ~1.5

// Lighting
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientColor;
uniform vec3 u_CameraPos;

// Point lights
struct PointLight {
    vec3 position;
    vec3 color;
    float radius;
    float constant;
    float linear;
    float quadratic;
};

#define MAX_POINT_LIGHTS 16
uniform int u_NumPointLights;
uniform PointLight u_PointLights[MAX_POINT_LIGHTS];

// Fresnel-Schlick approximation
float FresnelSchlick(vec3 viewDir, vec3 normal, float F0)
{
    float cosTheta = max(dot(viewDir, normal), 0.0);
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, u_FresnelPower);
}

void main()
{
    // Sample base texture
    vec4 baseColor = texture(u_BaseTexture, v_TexCoord);
    baseColor.rgb *= u_Color;
    
    // Apply mask texture if available
    float alpha = baseColor.a * (1.0 - u_Transparency);
    if (u_HasMaskTexture) {
        float mask = texture(u_MaskTexture, v_TexCoord).r;
        alpha *= mask;
    }
    
    // Discard fully transparent pixels
    if (alpha < 0.01) {
        discard;
    }
    
    // Normal mapping
    vec3 normal;
    if (u_HasNormalMap) {
        // Sample and decode normal map
        vec3 normalMap = texture(u_NormalTexture, v_TexCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0;
        
        if (u_FlipNormalMapY) {
            normalMap.y *= -1.0;
        }
        
        // Construct TBN matrix and transform normal
        mat3 TBN = mat3(normalize(v_Tangent), normalize(v_BiTangent), normalize(v_Normal));
        normal = normalize(TBN * normalMap);
    } else {
        normal = normalize(v_Normal);
    }
    
    // View direction
    vec3 viewDir = normalize(v_ViewPos - v_WorldPos);
    
    // Fresnel effect - more reflection at grazing angles
    float F0 = pow((u_RefractiveIndex - 1.0) / (u_RefractiveIndex + 1.0), 2.0);
    float fresnel = FresnelSchlick(viewDir, normal, F0);
    
    // Lighting calculations
    vec3 lightDir = normalize(u_LightDir);
    
    // Ambient
    vec3 ambient = u_AmbientColor * baseColor.rgb * 0.3;
    
    // Diffuse (attenuated for glass)
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = u_LightColor * diff * baseColor.rgb * 0.5;
    
    // Specular (glass is very shiny)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 64.0 / max(u_Roughness, 0.01));
    vec3 specular = u_LightColor * spec * 0.8;
    
    vec3 finalColor = ambient + diffuse + specular;
    
    // Point lights
    for(int i = 0; i < u_NumPointLights; i++)
    {
        float distance = length(u_PointLights[i].position - v_WorldPos);
        if(distance < u_PointLights[i].radius)
        {
            vec3 pLightDir = normalize(u_PointLights[i].position - v_WorldPos);
            
            // Diffuse
            float pDiff = max(dot(normal, pLightDir), 0.0);
            
            // Specular
            vec3 pHalfDir = normalize(pLightDir + viewDir);
            float pSpec = pow(max(dot(normal, pHalfDir), 0.0), 64.0);
            
            // Attenuation
            float attenuation = 1.0 / (u_PointLights[i].constant + 
                                       u_PointLights[i].linear * distance + 
                                       u_PointLights[i].quadratic * (distance * distance));
            
            vec3 pColor = u_PointLights[i].color * (pDiff * baseColor.rgb * 0.3 + pSpec * 0.5) * attenuation;
            finalColor += pColor;
        }
    }
    
    // Add fresnel rim lighting
    finalColor += fresnel * u_LightColor * 0.3;
    
    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0/2.2));
    
    // Output with calculated alpha
    // Blend between transparent glass and opaque based on fresnel
    float finalAlpha = mix(alpha, 1.0, fresnel * 0.3);
    
    FragColor = vec4(finalColor, finalAlpha);
}
