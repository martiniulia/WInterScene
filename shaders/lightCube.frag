#version 410 core
in vec3 FragPos;
in vec3 Normal;

out vec4 fColor;

uniform vec3 objectColor;
uniform vec3 viewPos;

void main()
{
    vec3 emissive = objectColor * 2.0;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 normal = normalize(Normal);
    float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 2.0);
    vec3 rimLight = fresnel * objectColor * 0.5;

    fColor = vec4(emissive + rimLight, 1.0);
}
