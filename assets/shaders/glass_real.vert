#version 330 core

// Glass Real Vertex Shader - matches BSP vertex data
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;
layout(location = 4) in vec2 aLightmapCoord;

out vec3 WorldPos;
out vec3 WorldNormal;
out vec2 TexCoord;
out vec2 LightmapCoord;
out vec3 VertexColor;
out vec3 ViewDir;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    vec4 worldPosition = u_Model * vec4(aPos, 1.0);
    WorldPos = worldPosition.xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
    WorldNormal = normalize(normalMatrix * aNormal);
    
    TexCoord = aTexCoord;
    LightmapCoord = aLightmapCoord;
    VertexColor = aColor;
    
    vec3 cameraPos = inverse(u_View)[3].xyz;
    ViewDir = normalize(WorldPos - cameraPos);
    
    gl_Position = u_Projection * u_View * worldPosition;
}
