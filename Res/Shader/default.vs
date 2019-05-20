#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoords;

out vec3 outNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    outNormal = mat3(transpose(inverse(model))) * inNormal;
    gl_Position = projection * view * model * vec4(inPos, 1.0);
}