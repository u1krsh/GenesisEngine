#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 ClipSpacePos;  // For screen-space refraction

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    vec4 worldPos = u_Model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    
    // Transform normal to world space
    Normal = mat3(transpose(inverse(u_Model))) * aNormal;
    
    TexCoord = aTexCoord;
    
    // Store clip-space position for screen-space effects
    ClipSpacePos = u_Projection * u_View * worldPos;
    gl_Position = ClipSpacePos;
}
