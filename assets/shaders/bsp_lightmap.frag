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
uniform int hasDiffuseTexture;
uniform int hasLightmap;
uniform int hasNormalMap;

void main() {
    // DEBUG: If hasNormalMap is set, output SOLID RED - unmissable!
    if (hasNormalMap != 0) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0); // SOLID RED
        return;
    }
    
    // Normal path for surfaces without normal maps
    float alpha = 1.0;
    vec3 diffuse = u_Color * VertexColor;
    
    if (hasDiffuseTexture != 0) {
        vec4 texColor = texture(diffuseTexture, TexCoord);
        diffuse *= texColor.rgb;
        alpha = texColor.a;
    }
    
    if (alpha < 0.01) {
        discard;
    }
    
    vec3 light = vec3(1.0);
    if (hasLightmap != 0) {
        light = texture(lightmapTexture, LightmapCoord).rgb;
    }
    
    vec3 result = diffuse * light;
    result = pow(result, vec3(1.0 / 2.2));
    
    FragColor = vec4(result, alpha);
}
