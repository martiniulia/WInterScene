#version 410 core

in vec3 Normal;
in vec2 passTexture;
in vec3 FragPos;
in vec4 FragPosLightSpace;
in vec4 fPosEye;

out vec4 fragmentColour;

// Texture samplers
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

// Directional light (the Sun)
uniform vec3 lightDir;
uniform vec3 lightColor; 

// Shadow map
uniform sampler2D shadowMap;

// Fog
uniform vec3 fogColor;
uniform float fogDensity;

// Smooth vs Solid control
uniform bool isSmooth;

// Point lights
uniform vec3 pointLightPos[8];
uniform vec3 pointLightColor[8];
uniform int numPointLights;

vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;

float shadow;

float computeShadow()
{
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = 0.010;
    float shadowFactor = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    if(projCoords.z > 1.0)
        shadowFactor = 0.0;

    return shadowFactor;
}

void computeLightComponents()
{    
    vec3 cameraPosEye = vec3(0.0f); // Viewer in eye space

    // Transform normal
    vec3 normalEye = normalize(Normal);    
    vec3 normal = isSmooth ? normalEye : normalize(cross(dFdx(FragPos), dFdy(FragPos)));

    // Directional light is already in world space
    vec3 lightDirN = normalize(lightDir);

    // View direction
    vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);

    // Ambient
    ambient = ambientStrength * lightColor;

    // Diffuse and specular
    if (isSmooth) {
        diffuse = max(dot(normal, lightDirN), 0.0f) * lightColor;

        vec3 reflection = reflect(-lightDirN, normal);
        float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
        specular = specularStrength * specCoeff * lightColor;
    } else {
        diffuse = max(dot(normal, lightDirN), 0.0f) * lightColor;
        specular = vec3(0.0);
    }

    // Shadow
    shadow = computeShadow();
}

void main()
{
    vec4 texColor = texture(diffuseTexture, passTexture);
    if (texColor.a < 0.1)
        discard;

    computeLightComponents();

    // Apply texture to directional light
    ambient *= texColor.rgb;
    diffuse *= texColor.rgb;
    specular *= texture(specularTexture, passTexture).rgb;

    vec3 directionalColor = min(ambient + (1.0 - shadow) * (diffuse + specular), 1.0);

    vec3 pointLightContribution = vec3(0.0);
    for (int i = 0; i < numPointLights; i++) {
        vec3 lightDirPoint = normalize(pointLightPos[i] - FragPos);
        float distance = length(pointLightPos[i] - FragPos);

        float constant = 1.0;
        float linear = 0.14;
        float quadratic = 0.07;
        float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

        vec3 normalEye = normalize(Normal);
        float diffPoint = max(dot(normalEye, lightDirPoint), 0.0);
        vec3 viewDir = normalize(-fPosEye.xyz);
        vec3 reflectDir = reflect(-lightDirPoint, normalEye);
        float specPoint = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
        vec3 specularPoint = specPoint * pointLightColor[i];

        pointLightContribution += (diffPoint * texColor.rgb + specularPoint) * pointLightColor[i] * attenuation;
    }

    vec3 finalColor = directionalColor + pointLightContribution;

   
    float distanceToCamera = length(fPosEye.xyz);
    float fogFactor = exp(-fogDensity * distanceToCamera);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 colorWithFog = mix(fogColor, finalColor, fogFactor);

    fragmentColour = vec4(colorWithFog, texColor.a);
}
