#version 330 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoords;

out vec4 FragColor;

uniform sampler2D u_Diffuse;
uniform vec3      u_ViewPos;

// Directional light
uniform vec3  u_DirLight_Direction;
uniform vec3  u_DirLight_Color;
uniform float u_DirLight_Intensity;

// Point light
uniform vec3  u_PointLight_Position;
uniform vec3  u_PointLight_Color;
uniform float u_PointLight_Intensity;
uniform float u_PointLight_Constant;
uniform float u_PointLight_Linear;
uniform float u_PointLight_Quadratic;

vec3 CalcDirLight(vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3  lightDir = normalize(-u_DirLight_Direction);
    float diff     = max(dot(normal, lightDir), 0.0);
    vec3  halfDir  = normalize(lightDir + viewDir);
    float spec     = pow(max(dot(normal, halfDir), 0.0), 32.0);

    vec3 ambient  = 0.25 * u_DirLight_Color * albedo;
    vec3 diffuse  = diff * u_DirLight_Color * albedo * u_DirLight_Intensity;
    vec3 specular = spec * u_DirLight_Color * u_DirLight_Intensity * 0.5;
    return ambient + diffuse + specular;
}

vec3 CalcPointLight(vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3  lightDir    = normalize(u_PointLight_Position - vFragPos);
    float diff        = max(dot(normal, lightDir), 0.0);
    vec3  halfDir     = normalize(lightDir + viewDir);
    float spec        = pow(max(dot(normal, halfDir), 0.0), 32.0);

    float dist        = length(u_PointLight_Position - vFragPos);
    float attenuation = 1.0 / (u_PointLight_Constant
                              + u_PointLight_Linear    * dist
                              + u_PointLight_Quadratic * dist * dist);

    vec3 diffuse  = diff * u_PointLight_Color * albedo * u_PointLight_Intensity * attenuation;
    vec3 specular = spec * u_PointLight_Color * u_PointLight_Intensity * attenuation * 0.5;
    return diffuse + specular;
}

void main()
{
    vec3 albedo  = texture(u_Diffuse, vTexCoords).rgb;
    vec3 normal  = normalize(vNormal);
    vec3 viewDir = normalize(u_ViewPos - vFragPos);

    vec3 result  = CalcDirLight(normal, viewDir, albedo);
    result      += CalcPointLight(normal, viewDir, albedo);

    FragColor = vec4(result, 1.0);
}
