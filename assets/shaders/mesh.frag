#version 330 core

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

out vec4 FragColor;

uniform vec3 u_Color;
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientColor;

uniform vec3 u_CameraPos;

struct PointLight {
    vec3 position;
    vec3 color; // includes intensity
    float radius;
    float constant;
    float linear;
    float quadratic;
};

#define MAX_POINT_LIGHTS 16
uniform int u_NumPointLights;
uniform PointLight u_PointLights[MAX_POINT_LIGHTS];

void main()
{
    // Normalize inputs
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(u_LightDir); // Directional light

    // 1. Ambient
    vec3 ambient = u_AmbientColor * u_Color;

    // 2. Directional Light (Diffuse)
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = u_LightColor * diff * u_Color;

    vec3 finalColor = ambient + diffuse;

    // 3. Point Lights
    for(int i = 0; i < u_NumPointLights; i++)
    {
        float distance = length(u_PointLights[i].position - v_WorldPos);
        if(distance < u_PointLights[i].radius)
        {
            // Diffuse
            vec3 pLightDir = normalize(u_PointLights[i].position - v_WorldPos);
            float pDiff = max(dot(normal, pLightDir), 0.0);
            
            // Attenuation
            float attenuation = 1.0 / (u_PointLights[i].constant + u_PointLights[i].linear * distance + u_PointLights[i].quadratic * (distance * distance));
            
            // Combine
            vec3 pColor = u_PointLights[i].color * pDiff * u_Color * attenuation;
            finalColor += pColor;
        }
    }

    FragColor = vec4(finalColor, 1.0);
}

