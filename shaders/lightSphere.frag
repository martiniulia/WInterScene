#version 330 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 viewPos;

void main()
{
    // Efect de lumină emissive - sfera pare să strălucească
    vec3 emissive = objectColor * 2.0; // Intensificăm culoarea pentru efect de lumină
    
    // Adăugăm un mic efect de fresnel pentru realism
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 normal = normalize(Normal);
    
    float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 2.0);
    vec3 rimLight = fresnel * objectColor * 0.5;
    
    // Combinăm emissive cu rim light
    vec3 finalColor = emissive + rimLight;
    
    FragColor = vec4(finalColor, 1.0);
}
