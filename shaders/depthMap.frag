#version 410 core

in vec2 fTexCoords;

uniform sampler2D diffuseTexture;

void main() 
{
    vec4 texColor = texture(diffuseTexture, fTexCoords);
    if (texColor.a < 0.1) {
        discard;
    }
}