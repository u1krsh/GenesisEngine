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
uniform float u_Alpha;  // For glass transparency (set by C++ code)
uniform bool hasDiffuseTexture;
uniform bool hasLightmap;

void main() {
    // Get alpha from uniform (glass) or default 1.0 (opaque)
    float alpha = u_Alpha;
    
    // Diffuse color from texture or base color
    vec3 diffuse = u_Color * VertexColor;
    if (hasDiffuseTexture) {
        vec4 texColor = texture(diffuseTexture, TexCoord);
        diffuse *= texColor.rgb;
        // Use texture alpha for cutout/transparency
        alpha *= texColor.a;
    }
    
    // Lightmap (pre-baked lighting)
    vec3 light = vec3(1.0);  // Full brightness fallback (for unlit/glass)
    if (hasLightmap) {
        light = texture(lightmapTexture, LightmapCoord).rgb;
    }
    
    // Final = diffuse * lightmap
    vec3 result = diffuse * light;
    
    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));
    
    FragColor = vec4(result, alpha);
}
