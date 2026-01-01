#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 u_View;
uniform mat4 u_Proj;
uniform float u_GridSize;
out vec3 v_WorldPos;
void main()
{
    v_WorldPos = aPos * u_GridSize;
    gl_Position = u_Proj * u_View * vec4(v_WorldPos, 1.0);
}
