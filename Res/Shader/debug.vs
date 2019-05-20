#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColor;

out vec4 outColor;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    outColor = inColor;
    gl_Position = projection * view * vec4(inPos, 1.0);
}