#version 330 core
in vec3 v_WorldPos;
out vec4 FragColor;
uniform vec3 u_GridColor;
uniform vec3 u_AxisColorX;
uniform vec3 u_AxisColorZ;
uniform float u_GridSpacing;
uniform float u_LineWidth;
uniform float u_FadeDistance;
void main()
{
    vec2 coord = v_WorldPos.xz / u_GridSpacing;
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    float line = min(grid.x, grid.y);
    float gridAlpha = 1.0 - min(line, 1.0);
    vec3 color = u_GridColor;
    float axisWidth = u_LineWidth * 2.0;
    if (abs(v_WorldPos.z) < axisWidth) {
        color = u_AxisColorX;
        gridAlpha = 1.0;
    }
    if (abs(v_WorldPos.x) < axisWidth) {
        color = u_AxisColorZ;
        gridAlpha = 1.0;
    }
    float dist = length(v_WorldPos.xz);
    float fade = 1.0 - smoothstep(u_FadeDistance * 0.5, u_FadeDistance, dist);
    FragColor = vec4(color, gridAlpha * fade * 0.5);
}
