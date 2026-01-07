#version 330 core

in vec2 TexCoord;
in vec2 LightmapCoord;
in vec3 VertexColor;

out vec4 FragColor;

// Textures
uniform sampler2D diffuseTexture;   // Texture unit 0
uniform sampler2D lightmapTexture;  // Texture unit 1

// Material
uniform vec3 u_Color;
uniform bool hasDiffuseTexture;
uniform bool hasLightmap;

void main() {
    // Diffuse color from texture or base color
    vec3 diffuse = u_Color * VertexColor;
    if (hasDiffuseTexture) {
        diffuse *= texture(diffuseTexture, TexCoord).rgb;
    }
    
    // Lightmap (pre-baked lighting)
    vec3 light = vec3(1.0);
    if (hasLightmap) {
        // Lightmap stores pre-multiplied lighting
        light = texture(lightmapTexture, LightmapCoord).rgb;
    }
    
    // Final = diffuse * lightmap
    vec3 result = diffuse * light;
    
    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));
    
    FragColor = vec4(result, 1.0);
}
