#version 330 core

out vec4 FragColor;

uniform vec3 objectColor; // culoarea cubului (galben cald)

void main()
{
    FragColor = vec4(objectColor, 1.0); // culoare pura, fara iluminare
}
