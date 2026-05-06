#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoords;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    vec4 worldPos   = u_Model * vec4(aPos, 1.0);
    vFragPos        = worldPos.xyz;
    vNormal         = mat3(transpose(inverse(u_Model))) * aNormal;
    vTexCoords      = aTexCoords;
    gl_Position     = u_Projection * u_View * worldPos;
}
