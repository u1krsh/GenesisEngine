#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;
layout(location = 4) in vec2 aLightmapCoord; // UV2: For future lightmap support

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 VertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    // Transform normal to world space
    Normal = mat3(transpose(inverse(model))) * aNormal;

    TexCoord = aTexCoord;
    VertexColor = aColor;

    gl_Position = projection * view * worldPos;
}

