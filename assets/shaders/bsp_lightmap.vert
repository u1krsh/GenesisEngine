#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;      // UV1: Diffuse texture
layout(location = 3) in vec3 aColor;
layout(location = 4) in vec2 aLightmapCoord; // UV2: Lightmap texture

out vec2 TexCoord;
out vec2 LightmapCoord;
out vec3 VertexColor;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

void main() {
    gl_Position = u_Proj * u_View * u_Model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    LightmapCoord = aLightmapCoord;
    VertexColor = aColor;
}
