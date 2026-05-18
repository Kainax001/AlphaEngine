#version 330 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoords;

out vec4 FragColor;

uniform sampler2D u_Diffuse;
uniform vec3      u_ViewPos;

layout(std140) uniform LightBlock {
    vec4 dirDirection;      // xyz = direction,        w = intensity
    vec4 dirColor;          // xyz = color,            w = 0 (padding)
    vec4 pointPosition;     // xyz = position,         w = intensity
    vec4 pointColor;        // xyz = color,            w = constant
    vec4 pointAttenuation;  // x = linear, y = quad,  zw = 0
    vec4 spotPosition;      // xyz = position,         w = intensity
    vec4 spotDirection;     // xyz = direction,        w = cutOff (cos)
    vec4 spotColor;         // xyz = color,            w = outerCutOff (cos)
    vec4 spotAttenuation;   // x = linear, y = quad,  z = constant, w = 0
} u_Lights;

// ---------------------------------------------------------------------------

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
    vec3  lightDir   = normalize(u_Lights.pointPosition.xyz - vFragPos);
    float diff       = max(dot(normal, lightDir), 0.0);
    vec3  halfDir    = normalize(lightDir + viewDir);
    float spec       = pow(max(dot(normal, halfDir), 0.0), 32.0);

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

vec3 CalcSpotLight(vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3  lightDir    = normalize(u_Lights.spotPosition.xyz - vFragPos);
    float theta       = dot(lightDir, normalize(-u_Lights.spotDirection.xyz));
    float cutOff      = u_Lights.spotDirection.w;
    float outerCutOff = u_Lights.spotColor.w;
    float epsilon     = cutOff - outerCutOff;
    float spotFactor  = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

    float diff    = max(dot(normal, lightDir), 0.0);
    vec3  halfDir = normalize(lightDir + viewDir);
    float spec    = pow(max(dot(normal, halfDir), 0.0), 32.0);

    float dist        = length(u_Lights.spotPosition.xyz - vFragPos);
    float constant_   = u_Lights.spotAttenuation.z;
    float linear_     = u_Lights.spotAttenuation.x;
    float quadratic_  = u_Lights.spotAttenuation.y;
    float attenuation = 1.0 / (constant_ + linear_ * dist + quadratic_ * dist * dist);

    float intensity = u_Lights.spotPosition.w;
    vec3  color     = u_Lights.spotColor.xyz;

    vec3 diffuse  = diff * color * albedo * intensity * attenuation * spotFactor;
    vec3 specular = spec * color * intensity * attenuation * spotFactor * 0.5;
    return diffuse + specular;
}

// ---------------------------------------------------------------------------

void main()
{
    vec3 albedo  = texture(u_Diffuse, vTexCoords).rgb;
    vec3 normal  = normalize(vNormal);
    vec3 viewDir = normalize(u_ViewPos - vFragPos);

    vec3 result  = CalcDirLight  (normal, viewDir, albedo);
    result      += CalcPointLight(normal, viewDir, albedo);
    result      += CalcSpotLight (normal, viewDir, albedo);

    FragColor = vec4(result, 1.0);
}
