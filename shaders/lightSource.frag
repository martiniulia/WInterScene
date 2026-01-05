#version 410 core

out vec4 fragmentColour;

uniform vec3 lightColor;

void main()
{
    fragmentColour = vec4(lightColor, 1.0);
}