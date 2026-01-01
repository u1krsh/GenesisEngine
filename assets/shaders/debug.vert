#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 u_View;
uniform mat4 u_Proj;
out vec3 v_Color;
void main()
{
    v_Color = aColor;
    gl_Position = u_Proj * u_View * vec4(aPos, 1.0);
}
