#version 330 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoords;

out vec4 FragColor;

uniform sampler2D u_Diffuse;
uniform vec3      u_ViewPos;

// ---------------------------------------------------------------------------
// 조명 UBO (binding = 1)
// C++ 쪽 LightUBOData 구조체와 std140 레이아웃이 1:1 대응한다.
//   dirDirection.w  = dirIntensity
//   dirColor.w      = 0 (padding)
//   pointPosition.w = pointIntensity
//   pointColor.w    = pointConstant
//   pointAttenuation.x = linear, .y = quadratic
// ---------------------------------------------------------------------------
layout(std140) uniform LightBlock {
    vec4 dirDirection;       // xyz = direction,  w = intensity
    vec4 dirColor;           // xyz = color,      w = unused
    vec4 pointPosition;      // xyz = position,   w = intensity
    vec4 pointColor;         // xyz = color,      w = constant
    vec4 pointAttenuation;   // x = linear, y = quadratic
} u_Lights;

vec3 CalcDirLight(vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3  lightDir = normalize(-u_Lights.dirDirection.xyz);
    float diff     = max(dot(normal, lightDir), 0.0);
    vec3  halfDir  = normalize(lightDir + viewDir);
    float spec     = pow(max(dot(normal, halfDir), 0.0), 32.0);

    float intensity = u_Lights.dirDirection.w;
    vec3  color     = u_Lights.dirColor.xyz;

    vec3 ambient  = 0.25 * color * albedo;
    vec3 diffuse  = diff * color * albedo * intensity;
    vec3 specular = spec * color * intensity * 0.5;
    return ambient + diffuse + specular;
}

vec3 CalcPointLight(vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3  lightDir    = normalize(u_Lights.pointPosition.xyz - vFragPos);
    float diff        = max(dot(normal, lightDir), 0.0);
    vec3  halfDir     = normalize(lightDir + viewDir);
    float spec        = pow(max(dot(normal, halfDir), 0.0), 32.0);

    float dist        = length(u_Lights.pointPosition.xyz - vFragPos);
    float constant_   = u_Lights.pointColor.w;
    float linear_     = u_Lights.pointAttenuation.x;
    float quadratic_  = u_Lights.pointAttenuation.y;
    float attenuation = 1.0 / (constant_ + linear_ * dist + quadratic_ * dist * dist);

    float intensity = u_Lights.pointPosition.w;
    vec3  color     = u_Lights.pointColor.xyz;

    vec3 diffuse  = diff * color * albedo * intensity * attenuation;
    vec3 specular = spec * color * intensity * attenuation * 0.5;
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
