#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 VertexColor;

out vec4 FragColor;

uniform vec3 viewPos;

// Material properties
uniform vec3 baseColor;
uniform float roughness;
uniform float metallic;
uniform bool hasBaseColorTexture;
uniform sampler2D baseColorTexture;

// Lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;

void main() {
    // Normalize inputs
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(-lightDir);  // Light direction points from light to surface

    // Base color
    vec3 albedo = baseColor;
    if (hasBaseColorTexture) {
        albedo *= texture(baseColorTexture, TexCoord).rgb;
    }
    albedo *= VertexColor;  // Multiply by vertex color

    // Simple Blinn-Phong lighting
    // Ambient
    vec3 ambient = ambientColor * albedo;

    // Diffuse
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = lightColor * albedo * NdotL;

    // Specular (Blinn-Phong)
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float shininess = mix(8.0, 128.0, 1.0 - roughness);
    float spec = pow(NdotH, shininess);
    vec3 specular = lightColor * spec * (1.0 - roughness) * 0.5;

    // Combine
    vec3 result = ambient + diffuse + specular;

    // Simple tone mapping
    result = result / (result + vec3(1.0));

    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));

    FragColor = vec4(result, 1.0);
}

