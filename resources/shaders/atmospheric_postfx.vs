#version 410 core

layout (location = 0)  in vec3 position;
layout (location = 1)  in vec2 texCoords;
layout (location = 2)  in vec3 normal;

smooth out vec2 fsTexCoords;

void main() {
    fsTexCoords = texCoords;
    gl_Position = vec4(position, 1.0);
}