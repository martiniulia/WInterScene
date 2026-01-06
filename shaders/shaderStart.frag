#version 410 core

in vec3 Normal;
in vec2 passTexture;
in vec3 FragPos;
in vec4 FragPosLightSpace;
in vec4 fPosEye;

out vec4 fragmentColour;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;

//shadow
uniform sampler2D shadowMap;

// fog
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
    // perform perspective divide
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // get closest depth value from light's perspective (using [0,1] range FragPosLightSpace as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // check if current fragment is in shadow
    float bias = 0.010;
    float shadowFactor = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum
    if(projCoords.z > 1.0)
        shadowFactor = 0.0;
    
    return shadowFactor;
}

void computeLightComponents()
{    
    vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
    
    //transform normal
    vec3 normalEye = normalize(Normal);    
    vec3 normal = isSmooth ? normalEye : normalize(cross(dFdx(FragPos), dFdy(FragPos)));
    
    //compute light direction
    vec3 lightDirN = normalize(lightDir);
    
    //compute view direction 
    vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
        
    //compute ambient light
    ambient = ambientStrength * lightColor;
    
    //compute diffuse light - different for smooth vs solid
    if (isSmooth) {
        // SMOOTH - full Phong lighting
        diffuse = max(dot(normal, lightDirN), 0.0f) * lightColor;
        
        //compute specular light
        vec3 reflection = reflect(-lightDirN, normal);
        float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
        specular = specularStrength * specCoeff * lightColor;
    } else {
        // SOLID - simple flat shading
        diffuse = max(dot(normal, lightDirN), 0.0f) * lightColor;
        specular = vec3(0.0f); // No specular for solid mode
    }
    
    //compute shadow
    shadow = computeShadow();
}

void main()
{
    vec4 texColor = texture(diffuseTexture, passTexture);
    if (texColor.a < 0.1)
        discard;

    computeLightComponents();
    
    // Apply texture to lighting components
    ambient *= texColor.rgb;
    diffuse *= texColor.rgb;
    specular *= texture(specularTexture, passTexture).rgb;

    // apply shadow: ambient is always present, diffuse and specular are reduced in shadow
    vec3 directionalColor = min(ambient + (1.0f - shadow) * (diffuse + specular), 1.0f);
    
    // Point lights contribution (not affected by shadows)
    vec3 pointLightContribution = vec3(0.0);
    for (int i = 0; i < numPointLights; i++) {
        vec3 lightDirPoint = normalize(pointLightPos[i] - FragPos);
        float distance = length(pointLightPos[i] - FragPos);
        
        // Attenuation pentru point lights
        float constant = 1.0;
        float linear = 0.22;
        float quadratic = 0.20;
        float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
        
        vec3 normalEye = normalize(Normal);
        float diffPoint = max(dot(normalEye, lightDirPoint), 0.0);
        
        // Simple point light without specular for simplicity
        pointLightContribution += diffPoint * pointLightColor[i] * texColor.rgb * attenuation * 1.5;
    }
    
    vec3 finalColor = directionalColor + pointLightContribution;

    // fog based on distance from camera in eye space
    float distanceToCamera = length(fPosEye.xyz);
    float fogFactor = exp(-fogDensity * distanceToCamera);
    fogFactor = clamp(fogFactor, 0.0f, 1.0f);

    vec3 colorWithFog = mix(fogColor, finalColor, fogFactor);
    
    fragmentColour = vec4(colorWithFog, texColor.a);
}