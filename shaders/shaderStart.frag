#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fPosLightSpace;

out vec4 fColor;

//lighting
uniform	vec3 lightDir;
uniform	vec3 lightColor;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

//shadow
uniform sampler2D shadowMap;

// shading mode: 0 = flat, 1 = smooth
uniform int shadingMode;

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
	vec3 projCoords = fPosLightSpace.xyz / fPosLightSpace.w;
	
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	
	// Check if fragment is outside the light's frustum
	if(projCoords.x < 0.0 || projCoords.x > 1.0 || 
	   projCoords.y < 0.0 || projCoords.y > 1.0 ||
	   projCoords.z < 0.0 || projCoords.z > 1.0)
	{
		return 0.0; // Fragment is outside light frustum, no shadow
	}
	
	// get closest depth value from light's perspective (using [0,1] range fPosLightSpace as coords)
	float closestDepth = texture(shadowMap, projCoords.xy).r;
	
	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	
	// check if current fragment is in shadow
	// Bias adaptiv bazat pe unghiul normalului față de direcția luminii
	// Folosim un bias mai mare pentru unghiuri mici pentru a evita shadow acne
	vec3 lightDirN = normalize(lightDir);
	vec3 normalEye = normalize(fNormal);
	float NdotL = dot(normalEye, lightDirN);
	// Bias mai mare când suprafața este aproape perpendiculară pe lumină
	float bias = max(0.005 * (1.0 - NdotL), 0.001);
	
	float shadowFactor = currentDepth - bias > closestDepth ? 1.0 : 0.0;
	
	return shadowFactor;
}

void computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye;
	if (shadingMode == 1) {
		// Smooth shading: folosim normala interpolată
		normalEye = normalize(fNormal);
	} else {
		// Flat shading: calculăm normala feței folosind derivatele
		vec3 dx = dFdx(fPosEye.xyz);
		vec3 dy = dFdy(fPosEye.xyz);
		normalEye = normalize(cross(dx, dy));
	}	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDir);
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
		
	//compute ambient light
	ambient = ambientStrength * lightColor;
	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
	
	//compute shadow
	shadow = computeShadow();
}

void main() 
{
	computeLightComponents();
	
	vec3 baseColor = vec3(0.9f, 0.35f, 0.0f);//orange
	
	vec4 diffuseTexColor = texture(diffuseTexture, fTexCoords);
	
	// discard completely transparent pixels to avoid weird blending artifacts
	if (diffuseTexColor.a < 0.1) {
		discard;
	}
	
	ambient *= diffuseTexColor.rgb;
	diffuse *= diffuseTexColor.rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;

	// apply shadow: ambient is always present, diffuse and specular are reduced in shadow
	// Pentru obiectele transparente (garduri PNG), aplicăm umbra proporțional cu opacitatea
	// Astfel umbra respectă forma gardului și nu este complet neagră
	float shadowFactor = shadow * diffuseTexColor.a; // Umbra este mai slabă pentru obiecte transparente
	vec3 color = min(ambient + (1.0f - shadowFactor) * (diffuse + specular), 1.0f);
    
    fColor = vec4(color, diffuseTexColor.a);
}