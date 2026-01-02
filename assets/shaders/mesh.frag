#version 330 core

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

out vec4 FragColor;

uniform vec3 u_Color;
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientColor;

void main()
{
    // Normalize inputs
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(u_LightDir);

    // Lambertian diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // Combine ambient and diffuse
    vec3 ambient = u_AmbientColor * u_Color;
    vec3 diffuse = u_LightColor * diff * u_Color;

    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0);
}

