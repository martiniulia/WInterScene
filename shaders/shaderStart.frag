#version 410 core

in vec3 Normal;
in vec2 passTexture;
in vec3 FragPos;
in vec4 FragPosLightSpace;
in vec4 fPosEye;

out vec4 fragmentColour;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

uniform vec3 lightDir;
uniform vec3 lightColor;

uniform vec3 pointLightPos[9];
uniform vec3 pointLightColor[9];
uniform float pointLightIntensity;
uniform int numPointLights;

uniform vec3 fogColor;
uniform float fogDensity;

uniform bool isSmooth;

float ambientStrength = 0.2;
float specularStrength = 0.5;
float shininess = 32.0;

float computeShadow()
{
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = 0.01;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    if (projCoords.z > 1.0) shadow = 0.0;

    return shadow;
}

vec3 computeSunLight(vec3 normal, vec3 viewDir, vec3 texColor)
{
    vec3 lightDirN = normalize(lightDir);

    vec3 ambient = ambientStrength * lightColor;
    vec3 diffuse = max(dot(normal, lightDirN), 0.0) * lightColor;

    vec3 reflection = reflect(-lightDirN, normal);
    float specCoeff = pow(max(dot(viewDir, reflection), 0.0), shininess);
    vec3 specular = specularStrength * specCoeff * lightColor;

    float shadow = computeShadow();

    return ambient * texColor +
           (1.0 - shadow) * (diffuse + specular) * texColor;
}

vec3 computePointLights(vec3 normal, vec3 fragPos, vec3 texColor)
{
    vec3 result = vec3(0.0);

    for (int i = 0; i < numPointLights; i++)
    {
        vec3 lightDirP = normalize(pointLightPos[i] - fragPos);
        float dist = length(pointLightPos[i] - fragPos);

        float constant = 1.0;
        float linear = 0.09;
        float quadratic = 0.032;
        float attenuation = 1.0 /
            (constant + linear * dist + quadratic * dist * dist);

        float diff = max(dot(normal, lightDirP), 0.0);
        result += diff * pointLightColor[i] * texColor * attenuation * pointLightIntensity;
    }

    return result;
}

void main()
{
    vec4 texColor = texture(diffuseTexture, passTexture);
    if (texColor.a < 0.1) discard;

    vec3 normal = isSmooth ? normalize(Normal)
                           : normalize(cross(dFdx(FragPos), dFdy(FragPos)));

    vec3 viewDir = normalize(-fPosEye.xyz);

    vec3 sunColor = computeSunLight(normal, viewDir, texColor.rgb);
    vec3 lanternColor = computePointLights(normal, FragPos, texColor.rgb);

    vec3 finalColor = sunColor + lanternColor;

    float dist = length(fPosEye.xyz);
    float fogFactor = exp(-fogDensity * dist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    finalColor = mix(fogColor, finalColor, fogFactor);

    fragmentColour = vec4(finalColor, texColor.a);
}
