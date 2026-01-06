#version 410 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 textcoord;

out vec3 Normal;
out vec2 passTexture;
out vec3 FragPos;
out vec4 FragPosLightSpace;
out vec4 fPosEye;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceTrMatrix;

void main()
{
    fPosEye = view * model * vec4(vertexPosition, 1.0f);
    FragPos = vec3(model * vec4(vertexPosition, 1.0));
    FragPosLightSpace = lightSpaceTrMatrix * model * vec4(vertexPosition, 1.0);

    Normal = normalize(normalMatrix * vertexNormal);
    passTexture = textcoord;

    gl_Position = projection * view * model * vec4(vertexPosition, 1.0);
}